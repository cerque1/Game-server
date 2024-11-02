#include "api_request_handler.h"

#include <boost/json.hpp>

namespace api_request_handler{

namespace json = boost::json;
using value_type = json::object::value_type;

namespace details{

    bool IsSubPath(fs::path path, fs::path base) {
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);

        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }  

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

    std::string ConvertGeoDirToMoveDir(model::DirectionGeo geo){
        switch (geo){
        case model::DirectionGeo::NORTH:
            return "U";
        case model::DirectionGeo::SOUTH:
            return "D";
        case model::DirectionGeo::WEST:
            return "L";
        case model::DirectionGeo::EAST:
            return "R";
        default:
            return "";
        }
    }

    model::DirectionGeo ConvertCharToDir(char dir){
        switch (dir)
        {
        case 'U':
            return model::DirectionGeo::NORTH;
        case 'D':
            return model::DirectionGeo::SOUTH;
        case 'L':
            return model::DirectionGeo::WEST;
        case 'R':
            return model::DirectionGeo::EAST;
        default:
            return model::DirectionGeo::NONE;
        }
    }

    std::optional<std::string> GetMoveDirection(const std::string& request_body){
        json::value move;
        try{
            move = json::parse(request_body);
        }catch(...){
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
        }catch(...){
            return std::nullopt;
        }
    }

    model::Speed MoveDirectionToSpeed(std::string move_dir, double dog_speed){
        if(move_dir.empty()){
            return model::Speed{0, 0};
        }

        char dir = move_dir[0];
        model::Speed dog_dir_speed_;
        switch (dir){
        case 'U':
            dog_dir_speed_.vertical = -dog_speed;
            break;
        case 'D':
            dog_dir_speed_.vertical = dog_speed;
            break;
        case 'L':
            dog_dir_speed_.horizontal = -dog_speed;
            break;
        case 'R': 
            dog_dir_speed_.horizontal = dog_speed;
            break;
        }

        return dog_dir_speed_;
    }

    bool IsValidActionBody(const std::optional<std::string>& move_dir){
        if(!move_dir.has_value()){
            return false;
        }

        if(move_dir->size() > 1){
            return false;
        }

        return true;
    }

    bool IsValidActionRequest(const StringRequest& request){
        auto content_type = request.find(http::field::content_type);
        if(content_type == request.end()){
            return false;
        }

        if(content_type->value() != request_handle_utils::ContentType::APPLICATION_JSON){
            return false;
        }

        return true;
    }

    StringResponse MakeUnauthorizeError(const std::string& code, const std::string& message, unsigned version, bool keep_alive){
        return request_handle_utils::MakeStringResponse(http::status::unauthorized, request_handle_utils::MakeErrorMessage(code, message),
                                                            version, keep_alive, request_handle_utils::ContentType::APPLICATION_JSON);
    }

    StringResponse MakeBadRequestError(const std::string& code, const std::string& message, unsigned version, bool keep_alive){
        return request_handle_utils::MakeStringResponse(http::status::bad_request, request_handle_utils::MakeErrorMessage(code, message),
                                                            version, keep_alive, request_handle_utils::ContentType::APPLICATION_JSON);
    }

    StringResponse MakeNotFoundError(const std::string& code, const std::string& message, unsigned version, bool keep_alive){
        return request_handle_utils::MakeStringResponse(http::status::not_found, request_handle_utils::MakeErrorMessage(code, message),
                                                            version, keep_alive, request_handle_utils::ContentType::APPLICATION_JSON);
    }

    StringResponse MakeNotAllowedMethodError(const std::string& code, const std::string& message, unsigned version, bool keep_alive, const std::string& allow){
        return request_handle_utils::MakeStringResponse(http::status::method_not_allowed, request_handle_utils::MakeErrorMessage(code, message),
                                                            version, keep_alive, request_handle_utils::ContentType::APPLICATION_JSON, allow);
    }

    std::optional<players::Token> TryExtractToken(const StringRequest& request){
        std::string auth;
        if(auto it = request.find(http::field::authorization); it != request.end()){
            auth = it->value();
        }

        std::string token; 
        size_t bearer = auth.find_first_of("Bearer");

        if(bearer == std::string::npos){
            return std::nullopt;
        }

        size_t token_start = auth.find_first_not_of(' ', bearer + 7);
        try{
            token = auth.substr(token_start, auth.find_first_of(' ', token_start) - token_start);
        }catch(...){
            return std::nullopt;
        }

        if(token.size() != 32){
            return std::nullopt;
        }

        return players::Token{token};
    }

    template <typename Func>
    StringResponse ExecuteAuthorized(Func&& action, const StringRequest& request){
        if(auto token = TryExtractToken(request); token.has_value()){
            return action(*token);
        }
        return MakeUnauthorizeError("invalidToken", "Authorization header is missing", request.version(), request.keep_alive());
    }
} // details

ApiRequestHandler::ApiRequestHandler(model::Game& game, int milliseconds, bool is_random_generate) : game_(game){
    game.SetRandomGenerate(is_random_generate);

    if(milliseconds == 0){
        is_test_version = true;
    }
    else{
        is_test_version = false;
    }
}

std::string ApiRequestHandler::GetMaps(){
    json::array maps;
    for(auto map : game_.GetMaps()){
        json::object map_obj;
        map_obj.insert(value_type("id", *map.GetId()));
        map_obj.insert(value_type("name", map.GetName()));
        maps.push_back(map_obj);
    }
    return json::serialize(maps);
}

std::string ApiRequestHandler::GetMapInfo(std::string_view id){
    const model::Map* map = game_.FindMap(model::Map::Id{std::string(id)});
    json::object map_obj;

    map_obj.insert(value_type("id", *map->GetId()));
    map_obj.insert(value_type("name", map->GetName()));

    map_obj.insert(value_type("roads", details::GetRoads(map->GetRoads())));    
    map_obj.insert(value_type("buildings", details::GetBuildings(map->GetBuildings())));
    map_obj.insert(value_type("offices", details::GetOffices(map->GetOffices())));

    return json::serialize(map_obj);
}

std::string ApiRequestHandler::MakeJsonAuthAnswer(std::string token, int player_id){
    json::object auth_answer;

    auth_answer.insert(value_type("authToken", token));
    auth_answer.insert(value_type("playerId", player_id));

    return json::serialize(auth_answer);
}

std::string ApiRequestHandler::FormJsonPlayersMap(const std::vector<std::pair<int, std::string>>& players){
    json::object players_json;

    for(auto [id, name] : players){
        json::object player; 
        player.insert(value_type("name", name));
        players_json.insert(value_type(std::to_string(id), player));
    }

    return json::serialize(players_json);
}

std::string ApiRequestHandler::FormJsonDogsInfo(const std::unordered_map<std::uint64_t, std::shared_ptr<model::Dog>>& dogs){
    json::object players;
    json::object dogs_json;

    for(auto [id, dog] : dogs){
        json::object dog_info;

        json::array coords;
        model::Coordinates dog_coords = dog->GetCoords();
        coords.push_back(dog_coords.x);
        coords.push_back(dog_coords.y);
        dog_info.insert(value_type("pos", coords));

        json::array speed;
        model::Speed dog_speed = dog->GetSpeed();
        speed.push_back(dog_speed.horizontal);
        speed.push_back(dog_speed.vertical);
        dog_info.insert(value_type("speed", speed));

        dog_info.insert(value_type("dir", details::ConvertGeoDirToMoveDir(dog->GetDir())));
        dogs_json.insert(value_type(std::to_string(id), dog_info));
    }
    players.insert(value_type("players", dogs_json));

    return json::serialize(players);
}

StringResponse ApiRequestHandler::GetPlayers(const StringRequest& request){
    if(request.method() != http::verb::get && request.method() != http::verb::head){
        return details::MakeNotAllowedMethodError("invalidMethod", "Invalid method", request.version(), request.keep_alive(), "GET, HEAD");
    }

    return details::ExecuteAuthorized([request, this](const players::Token& token){
                players::Player* player = players_.FindByToken(players::Token{token});
                if(player == nullptr){
                    return details::MakeUnauthorizeError("unknownToken", "Player token has not been found", request.version(), request.keep_alive());
                }

                return request_handle_utils::MakeStringResponse(http::status::ok, FormJsonPlayersMap(player->GetGameSession()->GetPlayersInfo()),
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
            }, 
            request);
}

StringResponse ApiRequestHandler::GetState(const StringRequest& request){
    if(request.method() != http::verb::get && request.method() != http::verb::head){
        return details::MakeNotAllowedMethodError("invalidMethod", "Invalid method", request.version(), request.keep_alive(), "GET, HEAD");
    }

    return details::ExecuteAuthorized([request, this](const players::Token& token){
                players::Player* player = players_.FindByToken(players::Token{token});
                if(player == nullptr){
                    return details::MakeUnauthorizeError("unknownToken", "Player token has not been found", request.version(), request.keep_alive());
                }

                return request_handle_utils::MakeStringResponse(http::status::ok, FormJsonDogsInfo(player->GetGameSession()->GetDogs()),
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
            }, 
            request);
}

StringResponse ApiRequestHandler::MakeAction(const StringRequest& request){
    if(request.method() != http::verb::post){
        return details::MakeNotAllowedMethodError("invalidMethod", "Invalid method", request.version(), request.keep_alive(), "POST");
    }

    return details::ExecuteAuthorized([request, this](const players::Token& token){
                players::Player* player = players_.FindByToken(players::Token{token});
                if(player == nullptr){
                    return details::MakeUnauthorizeError("unknownToken", "Player token has not been found", request.version(), request.keep_alive());
                }

                if(!details::IsValidActionRequest(request)){
                    return details::MakeBadRequestError("invalidArgument", "Invalid content type", request.version(), request.keep_alive());
                }

                std::optional<std::string> dog_move_dir = details::GetMoveDirection(request.body());
                if(!details::IsValidActionBody(dog_move_dir)){
                    return details::MakeBadRequestError("invalidArgument", "Failed to parse action", request.version(), request.keep_alive());
                }

                model::Speed dog_speed = details::MoveDirectionToSpeed(*dog_move_dir,
                                                                       player->GetGameSession()->GetMapDogSpeed());                
                player->SetDogSpeed(dog_speed);
                player->SetDogDir(details::ConvertCharToDir((*dog_move_dir)[0]));

                return request_handle_utils::MakeStringResponse(http::status::ok, "{}",
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
            }, 
            request);
}

StringResponse ApiRequestHandler::JoinPlayer(const StringRequest& request){
    if(request.method() != http::verb::post){
        return details::MakeNotAllowedMethodError("invalidMethod", "Only POST method is expected", request.version(), request.keep_alive(), "POST");
    }

    details::UserInfo user_info;
    try{
        user_info = details::GetUserInfoFromJson(request.body());
    }catch(...){
        return details::MakeBadRequestError("invalidArgument", "Join game request parse error", request.version(), request.keep_alive());
    }
    if(user_info.name_.empty()){
        return details::MakeBadRequestError("invalidArgument", "Invalid name", request.version(), request.keep_alive());
    }

    if(!IsContainsMap(user_info.map_id_)){
        return details::MakeNotFoundError("mapNotFound", "Map not found", request.version(), request.keep_alive());
    }

    model::GameSession* game_session = game_.FindGameSessionFromMapId(model::Map::Id(user_info.map_id_));
    if(game_session == nullptr){
        game_session = game_.AddGameSession(user_info.map_id_);
    }

    players::Player* player = players_.AddPlayer(user_info.name_, game_session);
    
    return request_handle_utils::MakeStringResponse(http::status::ok, MakeJsonAuthAnswer(*(player->GetToken()), player->GetId()),
                                    request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
}

StringResponse ApiRequestHandler::GetMapsResponse(const StringRequest& request, const std::string& target){
    if(request.method() == http::verb::get || request.method() == http::verb::head){
        size_t map_id_begin = target.find_first_of('/', 1);
        if(map_id_begin == std::string::npos){
            return request_handle_utils::MakeStringResponse(http::status::ok, GetMaps(), request.version(), 
                                    request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
        }
        else{
            std::string map_id = target.substr(map_id_begin);
            if(!IsContainsMap(map_id.substr(1))){
                return request_handle_utils::MakeStringResponse(http::status::not_found, request_handle_utils::MakeErrorMessage("mapNotFound", "Map not found"), 
                                        request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
            }
            else{
                return request_handle_utils::MakeStringResponse(http::status::ok, GetMapInfo(map_id.substr(1)), request.version(), 
                                        request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
            }
        }
    }
}

StringResponse ApiRequestHandler::SetTimeDelta(const StringRequest& request){
    if(request.method() != http::verb::post){
        return details::MakeNotAllowedMethodError("invalidMethod", "Only POST method is expected", request.version(), request.keep_alive(), "POST");
    }

    std::optional<int> time_delta = details::GetTimeDeltaFromJson(request.body());
    if(!time_delta.has_value()){
        return details::MakeBadRequestError("invalidArgument", "Failed to parse tick request JSON", request.version(), request.keep_alive());
    }

    game_.MakeActionsAtTime(*time_delta);
    return request_handle_utils::MakeStringResponse(http::status::ok, "{}",
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
}

} // api_request_handler
