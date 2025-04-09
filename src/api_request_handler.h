#pragma once

#include <iostream>
#include <filesystem>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>
#include <string>

#include "objects_collector.h"
#include "request_handle_utils.h"
#include "model.h"
#include "player.h"
#include "json_utils.h"
#include "extra_data.h"
#include "serializing_listener.h"
#include "postgres.h"

namespace fs = std::filesystem;

namespace api_request_handler{

namespace beast = boost::beast;
namespace http = beast::http;

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using Response = std::variant<StringResponse, FileResponse>;
using StringRequest = http::request<http::string_body>;

using namespace json_utils;
using namespace boost::urls;

namespace details{
    
    bool IsSubPath(fs::path path, fs::path base);
    
} // details

class ApiRequestHandler{
public:

    ApiRequestHandler(model::Game& game
                    , extra_data::LostObjectsOnMaps& lost_objects
                    , int milliseconds
                    , bool is_random_generate
                    , serializing_listener::ApplicationListener* app_listener
                    , std::shared_ptr<postgres::Database> db
                    , int retired_time);

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

                url_view uv(target);
                if(std::string(uv.encoded_path()) == "/records"){
                    int start = 0;
                    int max_items = 100;
                    if(uv.encoded_params().find("start") != uv.encoded_params().end()){
                        start = std::stoi(std::string(uv.encoded_params().find("start")->value));
                    }
                    if(uv.encoded_params().find("maxItems") != uv.encoded_params().end()){
                        max_items = std::stoi(std::string(uv.encoded_params().find("maxItems")->value));
                    }


                    return GetRecords(request
                                    , start
                                    , max_items);
                }
            }
        }
        return request_handle_utils::MakeStringResponse(http::status::bad_request, request_handle_utils::MakeErrorMessage("badRequest", "Bad request"), 
                                                request.version(), request.keep_alive(), request_handle_utils::ContentType::APPLICATION_JSON);
    }

    void Tick(int delta);

    const model::Game& GetGame(){
        return game_;
    }

    const players::Players& GetPlayers(){
        return players_;
    }

    const extra_data::LostObjectsOnMaps& GetLostObjects(){
        return lost_objects_;
    }

private:
    std::shared_ptr<postgres::Database> db_;
    model::Game& game_;
    extra_data::LostObjectsOnMaps& lost_objects_;
    players::Players players_;
    serializing_listener::ApplicationListener* app_listener_;
    int retired_time_;

    bool is_test_version;

    std::string GetMaps();
    std::string GetMapInfo(std::string_view id);

    std::string MakeJsonAuthAnswer(std::string token, int player_id);
    std::string FormJsonPlayersMap(const std::vector<std::pair<int, std::string>>& players);
    std::string FormJsonMapInfo(const std::unordered_map<std::uint64_t, std::shared_ptr<model::Dog>>& dogs, const std::string& map_id);
    std::string FormRecords(int start, int max_items) const;

    StringResponse GetPlayers(const StringRequest& request);
    StringResponse GetState(const StringRequest& request);
    StringResponse JoinPlayer(const StringRequest& request);
    StringResponse GetMapsResponse(const StringRequest& request, const std::string& target);
    StringResponse MakeAction(const StringRequest& request);
    StringResponse SetTimeDelta(const StringRequest& request);
    StringResponse GetRecords(const StringRequest& request, int start, int max_items) const;

    bool IsContainsMap(std::string_view id) const{
        return game_.FindMap(model::Map::Id{std::string(id)}) != nullptr;
    }
};

} // api_request_handler