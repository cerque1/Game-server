#pragma once

#include <boost/json.hpp>
#include <unordered_map>
#include <chrono>
#include <random>
#include <ctime>
#include <deque>

#include "model.h"
#include "loot_generator.h"

namespace extra_data{

using namespace std::chrono_literals;

struct LootObject{
    std::string name;
    std::string file_path;
    std::string type;
    std::optional<int> rotation;
    std::optional<std::string> color;
    double scale;
    int value;
};

struct LostObject{
    unsigned id;
    unsigned type;
    model::Coordinates coords;
};

// хранение информации о возможном луте на карте
class PossibleLootOnMapsToGenerate{
public:
    PossibleLootOnMapsToGenerate() = default;

    PossibleLootOnMapsToGenerate(std::chrono::milliseconds ms, double probability) : loot_generator_(ms, probability){}

    void AddPossibleLootToMap(std::string map_id, std::vector<LootObject> loot_types){
        possible_loot_on_map_[map_id] = std::move(loot_types);
    }

    std::vector<LootObject> GetPossibleLootObjectsOnMap(std::string_view id) const{
        return possible_loot_on_map_.at(std::string(id));
    }

    std::vector<LostObject> GenerateLootOnMap(const model::Map* map, int loot_count, int looter_count, int time);

    std::vector<std::string> GetMapsId() const;

    int GetObjectValue(const std::string& map_id, int obj_type) const{
        return possible_loot_on_map_.at(map_id).at(obj_type).value;
    }

private:

    unsigned GenerateRandomType(const std::string& map_id) const;

    std::unordered_map<std::string, std::vector<LootObject>> possible_loot_on_map_;
    loot_gen::LootGenerator loot_generator_;
    unsigned next_id_ = 0;
};

// хранение информации о луте находящемся на карте
class LostObjectsOnMaps{
public:
    LostObjectsOnMaps() = default;

    LostObjectsOnMaps& operator=(const LostObjectsOnMaps& other);

    explicit LostObjectsOnMaps(PossibleLootOnMapsToGenerate& possible_loot) 
                    : possible_loot_(possible_loot){
        for(const std::string& map_id : possible_loot_.GetMapsId()){
            lost_objects_on_map_[map_id] = {};
        }
    }

    void GenerateLostObjectsOnMaps(int delta, const model::Game& game);

    std::vector<LootObject> GetPossibleLootObjectsOnMap(const std::string& id) const{
        return possible_loot_.GetPossibleLootObjectsOnMap(id);
    }

    const std::vector<LostObject>& GetLostObjects(const std::string& id) const{
        return lost_objects_on_map_.at(id);
    }

    template <typename Iter>
    void EraseLostObjectsOnMap(const std::string& map_id, Iter begin, Iter end){
        int count_erased_objects = 0;
        for(Iter iter = begin; iter != end; ++iter){
            lost_objects_on_map_.at(map_id).erase(next(lost_objects_on_map_.at(map_id).begin(), *iter - count_erased_objects++));
        }
    }

    int GetObjectValue(const std::string& map_id, int obj_type) const{
        return possible_loot_.GetObjectValue(map_id, obj_type);
    }

    const std::unordered_map<std::string, std::vector<LostObject>>& GetLostObjectsOnMaps() const{
        return lost_objects_on_map_;
    }

    void SetLostObjectsOnMaps(std::unordered_map<std::string, std::vector<LostObject>>&& lost_objects){
        lost_objects_on_map_ = std::move(lost_objects);
    }

private:
    std::unordered_map<std::string, std::vector<LostObject>> lost_objects_on_map_;
    PossibleLootOnMapsToGenerate& possible_loot_;
};

} // extra_data namespace