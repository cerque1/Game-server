#include "extra_data.h"

namespace extra_data{

std::vector<LostObject> PossibleLootOnMapsToGenerate::GenerateLootOnMap(const model::Map* map, int loot_count, int looter_count, int time) {
    int count_new_lost_objects = loot_generator_.Generate(time * 1ms, loot_count, looter_count);

    std::vector<LostObject> new_lost_objects;
    for(int i = 0; i < count_new_lost_objects; ++i){
        new_lost_objects.push_back(LostObject{next_id_++, GenerateRandomType(*map->GetId()), model::generate_coords::GenerateRandomPointOnMap(*map)});
    }

    return new_lost_objects;
}

std::vector<std::string> PossibleLootOnMapsToGenerate::GetMapsId() const{
    std::vector<std::string> ids;
    for(auto [id, _] : possible_loot_on_map_){
        ids.push_back(id);
    }
    return ids;
}

unsigned PossibleLootOnMapsToGenerate::GenerateRandomType(const std::string& map_id) const{
    srand(time(NULL));
    return rand() % possible_loot_on_map_.at(map_id).size();
}

LostObjectsOnMaps& LostObjectsOnMaps::operator=(const LostObjectsOnMaps& other){
    lost_objects_on_map_ = other.lost_objects_on_map_;
    possible_loot_ = other.possible_loot_;
    return *this;
}       

void LostObjectsOnMaps::GenerateLostObjectsOnMaps(int delta, const model::Game& game){
    for(auto [map_id, lost_objects] : lost_objects_on_map_){
        auto game_session = game.FindGameSessionFromMapId(model::Map::Id{map_id});
        int looter_count = 0;
        if(game_session != nullptr){
            looter_count = game_session->GetDogsCount();
        }

        std::vector<LostObject> new_lost_objects = possible_loot_.GenerateLootOnMap(
                                    game.FindMap(model::Map::Id{map_id}), lost_objects.size(), looter_count, delta);

        for(LostObject lost_obj : new_lost_objects){
            if(lost_objects_on_map_[map_id].size() == lost_objects_on_map_[map_id].capacity()){
                lost_objects_on_map_.at(map_id).push_back(lost_obj);
            }
            else
                lost_objects_on_map_.at(map_id).push_back(lost_obj);

        }
    }
}

} // extra_data namespace