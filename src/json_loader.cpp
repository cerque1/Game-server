#include "json_loader.h"
#include "tagged.h"
#include <fstream>

namespace json_loader {

void AddRoadsToMap(model::Map& map, const boost::json::array& roads_json){
    for(auto road : roads_json){
        model::Point point{road.as_object().at("x0").as_int64(), road.as_object().at("y0").as_int64()};

        if(road.as_object().find("x1") != road.as_object().end()){
            map.AddRoad(model::Road{model::Road::HORIZONTAL, point, model::Coord{static_cast<int>(road.as_object().at("x1").as_int64())}});
        }
        else{
            map.AddRoad(model::Road{model::Road::VERTICAL, point, model::Coord{static_cast<int>(road.as_object().at("y1").as_int64())}});
        }
    }
}

void AddBuildingsToMap(model::Map& map, const boost::json::array& buildings_json){
    for(auto buildings : buildings_json){
        model::Point point{buildings.as_object().at(model::KeyLiterals::x).as_int64(), buildings.as_object().at(model::KeyLiterals::y).as_int64()};
        model::Size size{buildings.as_object().at(model::KeyLiterals::w).as_int64(), buildings.as_object().at(model::KeyLiterals::h).as_int64()};

        map.AddBuilding(model::Building{model::Rectangle{point, size}});
    }
}

void AddOfficesToMap(model::Map& map, const boost::json::array& offices_json){
    for(auto offices : offices_json){
        auto office_id_boost_str = offices.as_object().at("id").as_string();
        std::string office_id_std_str{office_id_boost_str.data(), office_id_boost_str.size()};

        model::Point point{offices.as_object().at(model::KeyLiterals::x).as_int64(), offices.as_object().at(model::KeyLiterals::y).as_int64()};
        model::Offset offset{offices.as_object().at("offsetX").as_int64(), offices.as_object().at("offsetY").as_int64()};

        map.AddOffice(model::Office{model::Office::Id{office_id_std_str}, point, offset});
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::fstream config(json_path, std::ios::in);
    if(!config.is_open()){
        throw std::invalid_argument("invalid path");
    }

    model::Game game;

    std::string config_str = "";
    std::string temp_str; 
    while(std::getline(config, temp_str)){
        config_str += temp_str;
    }

    auto value = boost::json::parse(config_str);
    double default_dog_speed = 1;
    if(value.as_object().find("defaultDogSpeed") != value.as_object().end()){
        default_dog_speed = value.as_object().at("defaultDogSpeed").as_double();
    }

    for(auto json_map : value.as_object().at("maps").as_array()){
        auto id_str = json_map.as_object().at("id").as_string();
        auto name = json_map.as_object().at("name").as_string();

        double dog_speed = default_dog_speed;
        if(json_map.as_object().find("dogSpeed") != json_map.as_object().end()){
            dog_speed = json_map.as_object().at("dogSpeed").as_double();
        }

        model::Map map(model::Map::Id{{std::string(id_str.data(), id_str.size())}}, std::string(name.data(), name.size()), dog_speed);

        AddRoadsToMap(map, json_map.as_object().at("roads").as_array());
        AddBuildingsToMap(map, json_map.as_object().at("buildings").as_array());
        AddOfficesToMap(map, json_map.as_object().at("offices").as_array());

        game.AddMap(map);
    }

    return game;
}

}  // namespace json_loader
