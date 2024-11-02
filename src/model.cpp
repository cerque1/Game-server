#include "model.h"

#include <stdexcept>
#include <random>
#include <ctime>

namespace model {
using namespace std::literals;

bool operator==(const Coordinates& lhs, const Coordinates& rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

namespace generate_coords{

double GenerateRandomDouble(int start, int end){
    return (std::rand() % ((start - end) * 10) + start * 10) / 10.; 
}

model::Coordinates GenerateRandomPointOnMap(const model::Map* map){
    model::Map::Roads roads = map->GetRoads();

    srand(std::time(NULL));
    model::Road road = roads[rand() % roads.size()];

    model::Coordinates coords;
    coords.x = road.GetStart().x == road.GetEnd().x ? road.GetStart().x : GenerateRandomDouble(road.GetStart().x, road.GetEnd().x);
    coords.y = road.GetStart().y == road.GetEnd().y ? road.GetStart().y : GenerateRandomDouble(road.GetStart().y, road.GetEnd().y);

    return coords;
}

} // generate_coords

Coordinates Map::CanGoToPoint(const Coordinates& A, const Coordinates& B, model::DirectionGeo dir) const{
    if(A == B || dir == model::DirectionGeo::NONE){
        return A;
    }   

    std::vector<Road> includes_roads;

    for(auto road : roads_){
        if(IsContainsInRoute(road, A)){
            includes_roads.push_back(road);

            if(IsContainsInRoute(road, B)){
                return B;
            }
        }
    }

    Coordinates coord;

    switch (dir){
    case model::DirectionGeo::NORTH:
        coord.x = A.x;
        for(auto road : includes_roads){
            if(coord.y > std::min(road.GetStart().y, road.GetEnd().y) - 0.4 &&
                std::min(road.GetStart().x, road.GetEnd().x) - 0.4 <= coord.x && 
                coord.x <= std::max(road.GetStart().x, road.GetEnd().x) + 0.4)
            {
                coord.y = std::min(road.GetStart().y, road.GetEnd().y) - 0.4;
            }
        }
        break;
    case model::DirectionGeo::SOUTH:
        coord.x = A.x;
        for(auto road : includes_roads){
            if(coord.y < std::max(road.GetStart().y, road.GetEnd().y) + 0.4 
                && std::min(road.GetStart().x, road.GetEnd().x) - 0.4 <= coord.x  
                && coord.x <= std::max(road.GetStart().x, road.GetEnd().x) + 0.4){
                coord.y = std::max(road.GetStart().y, road.GetEnd().y) + 0.4;
            }
        }
        break;
    case model::DirectionGeo::WEST:
        coord.y = A.y;
        for(auto road : includes_roads){
            if(coord.x > std::min(road.GetStart().x, road.GetEnd().x) - 0.4 
                && std::min(road.GetStart().y, road.GetEnd().y) - 0.4 <= coord.y 
                && coord.y <= std::max(road.GetStart().y, road.GetEnd().y) + 0.4){
                coord.x = std::min(road.GetStart().x, road.GetEnd().x) - 0.4;
            }
        }
        break;
    default:
        coord.y = A.y;
        for(auto road : includes_roads){
            if(coord.x < std::max(road.GetStart().x, road.GetEnd().x) + 0.4
                && std::min(road.GetStart().y, road.GetEnd().y) - 0.4 <= coord.y 
                && coord.y <= std::max(road.GetStart().y, road.GetEnd().y) + 0.4){
                coord.x = std::max(road.GetStart().x, road.GetEnd().x) + 0.4;
            }
        }
        break;
    }
    return coord;
}

bool Map::IsContainsInRoute(const Road& road, const Coordinates& point) const{
    return (std::min(road.GetStart().x, road.GetEnd().x) - 0.4 <= point.x && point.x <= std::max(road.GetStart().x, road.GetEnd().x) + 0.4) 
            && (std::min(road.GetStart().y, road.GetEnd().y) - 0.4 <= point.y && point.y <= std::max(road.GetStart().y, road.GetEnd().y) + 0.4);
}

std::shared_ptr<Dog> GameSession::AddDog(std::string name, int id){
    Coordinates dog_coord;
    if(is_random_generate_){
        dog_coord = generate_coords::GenerateRandomPointOnMap(map_);
    }
    dogs_.insert(std::pair<int, std::shared_ptr<Dog>>(id, std::make_shared<Dog>(id, name, dog_coord)));
    return dogs_[id];
}

std::vector<std::pair<int, std::string>> GameSession::GetPlayersInfo() const{
    std::vector<std::pair<int, std::string>> ids_;
    for(auto [id, dog] : dogs_){
        ids_.push_back({id, dog->GetName()});
    }
    return ids_;
}

void GameSession::MakeActionsAtTime(int time){
    for(auto [id, dog] : dogs_){
        Coordinates next_pos;
        next_pos.x = dog->GetCoords().x + dog->GetSpeed().horizontal * (time / 1000.);
        next_pos.y = dog->GetCoords().y + dog->GetSpeed().vertical * (time / 1000.);
        
        Coordinates move = map_->CanGoToPoint(dog->GetCoords(), next_pos, dog->GetDir());
        dog->SetCoords(move);
        if(move != next_pos){
            dog->SetSpeed(Speed{0, 0});
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

model::GameSession* Game::AddGameSession(std::string map_id){
    Map::Id map_id_{map_id};
    std::shared_ptr<model::GameSession> game_session = std::make_shared<model::GameSession>(FindMapForGameSession(map_id_), is_random_generate_);
    game_sessions_.push_back(game_session);
    game_session_to_map_id_[map_id_] = game_sessions_.size() - 1;
    return &*game_sessions_.back();
}

model::GameSession* Game::FindGameSessionFromMapId(const Map::Id& id) const{
    if (auto it = game_session_to_map_id_.find(id); it != game_session_to_map_id_.end()) {
        return &*game_sessions_.at(it->second);
    }
    return nullptr;
}

void Game::MakeActionsAtTime(int time){
    for(auto game_session : game_sessions_){
        game_session->MakeActionsAtTime(time);
    }
}

Map* Game::FindMapForGameSession(const Map::Id& id){
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

}  // namespace model
