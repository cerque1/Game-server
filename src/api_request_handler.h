#pragma once

#include <iostream>
#include <filesystem>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>

#include "request_handle_utils.h"
#include "model.h"
#include "player.h"

namespace fs = std::filesystem;

namespace api_request_handler{

namespace beast = boost::beast;
namespace http = beast::http;

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using Response = std::variant<StringResponse, FileResponse>;
using StringRequest = http::request<http::string_body>;

namespace details{

    struct UserInfo{
        std::string name_;
        std::string map_id_;
        
        UserInfo() = default;
        UserInfo(std::string name, std::string map_id) : name_(name), map_id_(map_id){}
    };
    
    bool IsSubPath(fs::path path, fs::path base);

    UserInfo GetUserInfoFromJson(std::string json_doc);
    
} // details

class ApiRequestHandler{
public:

    ApiRequestHandler(model::Game& game, int milliseconds, bool is_random_generate);

    template <typename SomeRequest>
    Response HandleRequest(SomeRequest&& request, std::string target){
            if(details::IsSubPath(target, "/v1")){
                target = target.substr(target.find_first_of('/', 1));
                if(details::IsSubPath(target, "/maps")){
                    return GetMapsResponse(request, target);
                }
                else if(details::IsSubPath(target, "/game")){
                    target = target.substr(target.find_first_of('/', 1));
                    if(details::IsSubPath(target, "/join")){
                        return JoinPlayer(request);
                    }
                    else if(details::IsSubPath(target, "/player")){
                        target = target.substr(target.find_first_of('/', 1));
                        if(details::IsSubPath(target, "/action")){
                            return MakeAction(request);
                        }
                    }
                    else if(details::IsSubPath(target, "/players")){
                        return GetPlayers(request);
                    }
                    else if(details::IsSubPath(target, "/state")){
                        return GetState(request);
                    }
                    else if(details::IsSubPath(target, "/tick")){
                        if(is_test_version){
                            return SetTimeDelta(request);
                        }
                        return request_handle_utils::MakeStringResponse(http::status::bad_request, request_handle_utils::MakeErrorMessage("invalidMethod", "Invalid endpoint"), 
                                                    request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
                    }
                }
            
        }
        else{
            return request_handle_utils::MakeStringResponse(http::status::bad_request, request_handle_utils::MakeErrorMessage("badRequest", "Bad request"), 
                                                    request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
        }
    }

    void Tick(int delta){
        game_.MakeActionsAtTime(delta);
    }
private:
    model::Game& game_;
    players::Players players_;

    bool is_test_version;

    std::string GetMaps();
    std::string GetMapInfo(std::string_view id);

    std::string MakeJsonAuthAnswer(std::string token, int player_id);
    std::string FormJsonPlayersMap(const std::vector<std::pair<int, std::string>>& players);
    std::string FormJsonDogsInfo(const std::unordered_map<std::uint64_t, std::shared_ptr<model::Dog>>& dogs);

    StringResponse GetPlayers(const StringRequest& request);
    StringResponse GetState(const StringRequest& request);
    StringResponse JoinPlayer(const StringRequest& request);
    StringResponse GetMapsResponse(const StringRequest& request, const std::string& target);
    StringResponse MakeAction(const StringRequest& request);
    StringResponse SetTimeDelta(const StringRequest& request);

    bool IsContainsMap(std::string_view id){
        return game_.FindMap(model::Map::Id{std::string(id)}) != nullptr;
    }
};

} // api_request_handler