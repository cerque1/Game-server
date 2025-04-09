#include "json_utils.h"

namespace json_utils{
    using value_type = json::object::value_type;

    UserInfo GetUserInfoFromJson(std::string json_doc){
        auto value = boost::json::parse(json_doc).as_object();
        std::string user_name = std::string(value.at("userName").as_string());
        std::string map_id = std::string(value.at("mapId").as_string());
        return UserInfo{user_name, map_id};
    } 

    json::array GetRoads(const model::Map::Roads& roads){
        json::array roads_obj;

        for(auto road : roads){
            json::object road_info;
            road_info.insert(value_type("x0", road.GetStart().x));
            road_info.insert(value_type("y0", road.GetStart().y));

            if(road.IsHorizontal()){
                road_info.insert(value_type("x1", road.GetEnd().x));
            }
            else{
                road_info.insert(value_type("y1", road.GetEnd().y));
            }

            roads_obj.push_back(road_info);
        }
        return roads_obj;
    }

    json::array GetBuildings(const model::Map::Buildings& buildings){
        json::array buildings_obj;

        for(auto building : buildings){
            json::object building_info;

            building_info.insert(value_type(model::KeyLiterals::x, building.GetBounds().position.x));
            building_info.insert(value_type(model::KeyLiterals::y, building.GetBounds().position.y));
            building_info.insert(value_type(model::KeyLiterals::w, building.GetBounds().size.width));
            building_info.insert(value_type(model::KeyLiterals::h, building.GetBounds().size.height));

            buildings_obj.push_back(building_info);
        }
        return buildings_obj;
    }

    json::array GetOffices(const model::Map::Offices& offices){
        json::array offices_obj;

        for(auto office : offices){
            json::object office_info;

            office_info.insert(value_type("id", *office.GetId()));
            office_info.insert(value_type(model::KeyLiterals::x, office.GetPosition().x));
            office_info.insert(value_type(model::KeyLiterals::y, office.GetPosition().y));
            office_info.insert(value_type("offsetX", office.GetOffset().dx));
            office_info.insert(value_type("offsetY", office.GetOffset().dy));

            offices_obj.push_back(office_info);
        }
        return offices_obj;
    }

    json::array GetLootTypes(const std::vector<extra_data::LootObject>& loot_objects){
        json::array loot_objects_array;
        for(extra_data::LootObject loot_obj : loot_objects){
            json::object loot_info;

            loot_info.insert(value_type("name", loot_obj.name));
            loot_info.insert(value_type("file", loot_obj.file_path));
            loot_info.insert(value_type("type", loot_obj.type));
            if(loot_obj.rotation.has_value()){
                loot_info.insert(value_type("rotation", loot_obj.rotation.value()));
            }
            if(loot_obj.color.has_value()){
                loot_info.insert(value_type("color", loot_obj.color.value()));
            }
            loot_info.insert(value_type("scale", loot_obj.scale));
            loot_info.insert(value_type("value", loot_obj.value));

            loot_objects_array.push_back(loot_info);
        }

        return loot_objects_array;
    }

    std::optional<std::string> GetMoveDirection(const std::string& request_body){
        json::value move;
        try{
            move = json::parse(request_body);
        }catch(std::exception const& e){
            return std::nullopt;
        }

        if(move.as_object().find("move") == move.as_object().end()){
            return std::nullopt;
        }
        return std::string(move.as_object().at("move").as_string());
    }

    std::optional<int> GetTimeDeltaFromJson(const std::string& request_body){
        try{
            json::value time_delta = json::parse(request_body);
            return time_delta.as_object().at("timeDelta").as_int64();
        }catch(std::exception const& e){
            return std::nullopt;
        }
    }
}