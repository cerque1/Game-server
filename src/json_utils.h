#pragma once
#include <boost/json.hpp>
#include "model.h"
#include "extra_data.h"

namespace json_utils{
    namespace json = boost::json;

    struct UserInfo{
        std::string name_;
        std::string map_id_;
        
        UserInfo() = default;
        UserInfo(std::string name, std::string map_id) : name_(name), map_id_(map_id){}
    };

    UserInfo GetUserInfoFromJson(std::string json_doc);
    json::array GetRoads(const model::Map::Roads& roads);
    json::array GetBuildings(const model::Map::Buildings& buildings);
    json::array GetOffices(const model::Map::Offices& offices);
    json::array GetLootTypes(const std::vector<extra_data::LootObject>& loot_objects);
    std::optional<std::string> GetMoveDirection(const std::string& request_body);
    std::optional<int> GetTimeDeltaFromJson(const std::string& request_body);
}