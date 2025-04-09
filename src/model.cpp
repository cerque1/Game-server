#include "model.h"

#include <stdexcept>
#include <random>
#include <ctime>
#include <chrono>

#include "records.h"

namespace model {
using namespace std::literals;

constexpr double EPSILON = 10e-6;

bool operator==(const Coordinates& lhs, const Coordinates& rhs){
    return std::abs(lhs.x - rhs.x) < EPSILON
        && std::abs(lhs.y - rhs.y) < EPSILON;
}

bool operator==(const Speed& lhs, const Speed& rhs){
    return std::abs(lhs.horizontal - rhs.horizontal) < EPSILON
        && std::abs(lhs.vertical - rhs.vertical) < EPSILON;
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

model::Coordinates GenerateRandomPointOnMap(const model::Map& map){
    model::Map::Roads roads = map.GetRoads();

    model::Road road = roads[rand() % roads.size()];

    model::Coordinates coords;
    coords.x = road.GetStart().x == road.GetEnd().x ? 
                                    road.GetStart().x : 
                                    GenerateRandomDouble(std::min(road.GetStart().x, road.GetEnd().x), std::max(road.GetStart().x, road.GetEnd().x));
    coords.y = road.GetStart().y == road.GetEnd().y ? 
                                    road.GetStart().y : 
                                    GenerateRandomDouble(std::min(road.GetStart().y, road.GetEnd().y), std::max(road.GetStart().y, road.GetEnd().y));

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

    coord.x = A.x;
    coord.y = A.y;

    switch (dir){
    case model::DirectionGeo::NORTH:
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
        for(auto road : includes_roads){
            if(coord.y < std::max(road.GetStart().y, road.GetEnd().y) + 0.4 
                && std::min(road.GetStart().x, road.GetEnd().x) - 0.4 <= coord.x  
                && coord.x <= std::max(road.GetStart().x, road.GetEnd().x) + 0.4){
                coord.y = std::max(road.GetStart().y, road.GetEnd().y) + 0.4;
            }
        }
        break;
    case model::DirectionGeo::WEST:
        for(auto road : includes_roads){
            if(coord.x > std::min(road.GetStart().x, road.GetEnd().x) - 0.4 
                && std::min(road.GetStart().y, road.GetEnd().y) - 0.4 <= coord.y 
                && coord.y <= std::max(road.GetStart().y, road.GetEnd().y) + 0.4){
                coord.x = std::min(road.GetStart().x, road.GetEnd().x) - 0.4;
            }
        }
        break;
    default:
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
        dog_coord = generate_coords::GenerateRandomPointOnMap(*map_);
    }
    dogs_.insert(std::pair<int, std::shared_ptr<Dog>>(id, std::make_shared<Dog>(id, name, dog_coord)));
    return dogs_[id];
}

void GameSession::AddDog(uint64_t id, const Dog& dog){
    dogs_[id] = std::make_shared<Dog>(dog);
}

std::vector<std::pair<int, std::string>> GameSession::GetPlayersInfo() const{
    std::vector<std::pair<int, std::string>> ids_;
    for(auto [id, dog] : dogs_){
        ids_.push_back({id, dog->GetName()});
    }
    return ids_;
}

std::unordered_map<int, MoveInfo> GameSession::MakeActionsAtTime(int time){
    std::unordered_map<int, MoveInfo> moves_info;

    for(auto [id, dog] : dogs_){
        Coordinates next_pos;
        next_pos.x = dog->GetCoords().x + dog->GetSpeed().horizontal * (time / 1000.);
        next_pos.y = dog->GetCoords().y + dog->GetSpeed().vertical * (time / 1000.);
        
        Coordinates move = map_->CanGoToPoint(dog->GetCoords(), next_pos, dog->GetDir());
        Coordinates last_pos = dog->GetCoords();

        if(last_pos == move && !dog->GetChangeDirInTick()){
            dog->AddAfkTime(time);
        }
        else{
            dog->ClearAfkTime();
        }

        dog->SetChangeDirInTick(false);
        dog->SetCoords(move);
        if(move != next_pos){
            dog->SetSpeed(Speed{0, 0});
        }

        dog->AddPlayTime(time);
        moves_info[dog->GetId()] = {last_pos, next_pos};
    }

    return moves_info;
}

void GameSession::EraseDogsById(std::vector<int> dogs_id, std::vector<domain::Record>& records){
    for(int dog_id : dogs_id){
        auto dog = dogs_[dog_id];
        records.push_back(domain::Record{domain::RecordId::New(), dog->GetName(), dog->GetScore(), dog->GetPlayTime()});
        dogs_.erase(dog_id);
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

void Game::AddGameSession(GameSession&& game_sessoion){
    game_sessions_.push_back(std::make_shared<GameSession>(std::move(game_sessoion)));
    game_session_to_map_id_[Map::Id(game_sessoion.GetMapId())] = game_sessions_.size() - 1;
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

MapIdToMovesInfo Game::MakeActionsAtTime(int time){
    MapIdToMovesInfo map_id_to_moves_info;

    for(auto game_session : game_sessions_){
        std::unordered_map<int, MoveInfo> moves_info = game_session->MakeActionsAtTime(time);
        map_id_to_moves_info[Map::Id(game_session->GetMapId())] = moves_info;
    }

    return map_id_to_moves_info;
}

Map* Game::FindMapForGameSession(const Map::Id& id){
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

std::vector<domain::Record> Game::EraseRetiredDogs(std::unordered_map<std::string, std::vector<int>> dogs_id){
    std::vector<domain::Record> records_res;

    for(auto [map_id, dogs_id_on_map] : dogs_id){
        auto game_session_ = FindGameSessionFromMapId(model::Map::Id(map_id));
        game_session_->EraseDogsById(dogs_id_on_map, records_res);
    }

    return records_res;
}

}  // namespace model
