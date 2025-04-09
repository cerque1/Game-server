#include "api_request_handler.h"
#include "log_utils.h"

#include <boost/json.hpp>
#include <boost/archive/binary_iarchive.hpp>

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

    model::Speed MoveDirectionToSpeed(char move_dir, double dog_speed){
        if(move_dir == '\0'){
            return model::Speed{0, 0};
        }

        model::Speed dog_dir_speed_;
        switch (move_dir){
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
        default:
            assert(false);
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

ApiRequestHandler::ApiRequestHandler(model::Game& game
                        , extra_data::LostObjectsOnMaps& lost_objects
                        , int milliseconds
                        , bool is_random_generate
                        , serializing_listener::ApplicationListener* app_listener
                        , std::shared_ptr<postgres::Database> db
                        , int retired_time)
                        : game_(game), lost_objects_(lost_objects), app_listener_(app_listener), db_(db), retired_time_(retired_time){
    game_.SetRandomGenerate(is_random_generate);
    
    if(app_listener_ != nullptr){
        std::ifstream in_file(dynamic_cast<serializing_listener::SerializingListener*>(app_listener_)->GetPath(), std::ios::binary | std::ios::in);
        if(in_file.is_open()){
            try{
                boost::archive::binary_iarchive ar{in_file};

                std::vector<serialization::GameSessionRepr> game_sessions_repr;
                serialization::PlayersRepr players_repr;
                std::unordered_map<std::string, std::vector<serialization::LostObjectRepr>> lost_objects_repr;            

                ar >> game_sessions_repr;
                ar >> players_repr;
                ar >> lost_objects_repr;

                for(auto game_session_repr : game_sessions_repr){
                    game_.AddGameSession(game_session_repr.Restore(game_, is_random_generate));
                }
                players_ = players_repr.Restore(game_);

                std::unordered_map<std::string, std::vector<extra_data::LostObject>> lost_objs;
                for(auto [map_id, lost_obj] : lost_objects_repr){
                    lost_objs[map_id] = {};
                    for(auto object : lost_obj){
                        lost_objs[map_id].push_back(object.Restore());
                    }
                }
                lost_objects_.SetLostObjectsOnMaps(std::move(lost_objs));
            }
            catch(std::exception& e){
                BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                        << logging::add_value(additional_data, log_data::MakeServerExitedData(1, e.what()))
                                        << "server exited with deserialize error";
                throw e;
            }
        }
    }

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

    map_obj.insert(value_type("roads", GetRoads(map->GetRoads())));    
    map_obj.insert(value_type("buildings", GetBuildings(map->GetBuildings())));
    map_obj.insert(value_type("offices", GetOffices(map->GetOffices())));
    map_obj.insert(value_type("lootTypes", GetLootTypes(lost_objects_.GetPossibleLootObjectsOnMap(std::string(id)))));

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

std::string ApiRequestHandler::FormJsonMapInfo(const std::unordered_map<std::uint64_t, std::shared_ptr<model::Dog>>& dogs, const std::string& map_id){
    json::object map_info;

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

        json::array bag;
        for(auto item : dog->GetBag()){
            json::object item_object;
            item_object.insert(value_type("id", item.id));
            item_object.insert(value_type("type", item.type));

            bag.push_back(item_object);
        }

        dog_info.insert(value_type("bag", bag));
        dog_info.insert(value_type("score", dog->GetScore()));

        dogs_json.insert(value_type(std::to_string(id), dog_info));
    }
    map_info.insert(value_type("players", dogs_json));

    json::object lost_objects_json;
    for(auto lost_obj : lost_objects_.GetLostObjects(map_id)){
        json::object lost_obj_json;

        json::array coords;
        coords.push_back(lost_obj.coords.x);
        coords.push_back(lost_obj.coords.y);

        lost_obj_json.insert(value_type("pos", coords));
        lost_obj_json.insert(value_type("type", lost_obj.type));
        lost_objects_json.insert(value_type(std::to_string(lost_obj.id), lost_obj_json));
    }
    map_info.insert(value_type("lostObjects", lost_objects_json));

    return json::serialize(map_info);
}

std::string ApiRequestHandler::FormRecords(int start, int max_items) const{
    json::array records_json;

    auto records = db_->GetRecordRepo()->GetRecords(start, max_items);
    for(auto record : records){
        json::object record_json;
        record_json.insert(json::object::value_type("name", record.GetDogName()));
        record_json.insert(json::object::value_type("score", record.GetScore()));
        record_json.insert(json::object::value_type("playTime", record.GetTime()/1000.));
        records_json.push_back(record_json);
    }

    return json::serialize(records_json);
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
                
                return request_handle_utils::MakeStringResponse(http::status::ok, FormJsonMapInfo(player->GetGameSession()->GetDogs(), player->GetMapId()),
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

                std::optional<std::string> dog_move_dir = GetMoveDirection(request.body());
                if(!details::IsValidActionBody(dog_move_dir)){
                    return details::MakeBadRequestError("invalidArgument", "Failed to parse action", request.version(), request.keep_alive());
                }

                model::Speed dog_speed = details::MoveDirectionToSpeed((*dog_move_dir)[0],
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

    UserInfo user_info;
    try{
        user_info = GetUserInfoFromJson(request.body());
    }catch(std::exception const& e){
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
    return details::MakeNotAllowedMethodError("invalidMethod", "Invalid method", request.version(), request.keep_alive(), "GET, HEAD");
}

StringResponse ApiRequestHandler::SetTimeDelta(const StringRequest& request){
    if(request.method() != http::verb::post){
        return details::MakeNotAllowedMethodError("invalidMethod", "Only POST method is expected", request.version(), request.keep_alive(), "POST");
    }

    std::optional<int> time_delta = GetTimeDeltaFromJson(request.body());
    if(!time_delta.has_value()){
        return details::MakeBadRequestError("invalidArgument", "Failed to parse tick request JSON", request.version(), request.keep_alive());
    }

    Tick(*time_delta);
    return request_handle_utils::MakeStringResponse(http::status::ok, "{}",
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
}

StringResponse ApiRequestHandler::GetRecords(const StringRequest& request, int start, int max_items) const{
    if(start < 0 || max_items < 0 || max_items > 100){
        return details::MakeBadRequestError("invalidArgument", "Invalid arguments", request.version(), request.keep_alive());
    }
    return request_handle_utils::MakeStringResponse(http::status::ok, FormRecords(start, max_items),
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
}

void ApiRequestHandler::Tick(int delta){
    auto moves_info = game_.MakeActionsAtTime(delta);
    lost_objects_.GenerateLostObjectsOnMaps(delta, game_);
    objects_collector::CollectObjects(game_, lost_objects_, moves_info);
    auto retired_dogs_id = players_.EraseRetiredPlayers(retired_time_);
    auto records_result = game_.EraseRetiredDogs(retired_dogs_id);

    if(!records_result.empty()){
        db_->GetRecordRepo()->SaveRecords(records_result);
    }

    if(app_listener_){
        app_listener_->OnTick(delta * 1ms, game_, players_, lost_objects_);
    }
}

} // api_request_handler
