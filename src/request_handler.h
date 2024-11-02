#pragma once

#include "http_server.h"
#include "model.h"
#include "log_utils.h"
#include "api_request_handler.h"

#include <iostream>
#include <filesystem>
#include <variant>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace api = api_request_handler;

using namespace std::literals;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileRequest = http::request<http::file_body>;
using FileResponse = http::response<http::file_body>;

using Response = std::variant<StringResponse, FileResponse>;

struct ContentType {
    ContentType() = delete;

    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv; 
    constexpr static std::string_view TEXT_CSS = "text/css"sv;
    constexpr static std::string_view TEXT_JS = "text/javascript"sv;
    constexpr static std::string_view TEXT = "text/plain"sv;
    constexpr static std::string_view APPLICATION_XML = "application/xml"sv;
    constexpr static std::string_view IMAGE_PNG = "image/png"sv;
    constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;
    constexpr static std::string_view IMAGE_GIF = "image/gif"sv;
    constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;
    constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;
    constexpr static std::string_view IMAGE_SVG_XML = "image/svg+xml"sv;
    constexpr static std::string_view AUDIO_MP3 = "audio/mpeg"sv;
    constexpr static std::string_view APPLICATION_OCTET_STREAM = "application/octet-stream"sv;
};

class LoggingRequestHandle {

    template <typename SomeResponse>
    void OnLogResponse(const SomeResponse& response){
        std::string content_type;
        if(auto iter = response.find(http::field::content_type); iter != response.end()){
            content_type = iter->value();
        }

        BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                            << logging::add_value(additional_data, log_data::MakeResponseSendData(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_handle).count(),
                                                                             static_cast<boost::beast::http::status>(response.result_int()), content_type))
                            << "response sent";
    }

    void LogResponse();
public:
    LoggingRequestHandle(const Response& response_) : response_(response_), start_handle(std::chrono::system_clock::now()){}

    ~LoggingRequestHandle(){
        LogResponse();
    }

private:
    const Response& response_;
    std::chrono::time_point<std::chrono::system_clock> start_handle;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    explicit RequestHandler(model::Game& game, fs::path&& root, Strand api_strand, int milliseconds, bool is_random_generate)
        : api_request_handler_(game, milliseconds, is_random_generate), root_(std::move(root)), api_strand_(api_strand) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send){
        StringRequest request(std::forward<decltype(req)>(req));

        std::string target{DecodingURI(request.target())};
        fs::path full_path = fs::weakly_canonical(fs::path(root_.string() + target));

        //Проверка нахождения пути в корневом каталоге
        if(!api::details::IsSubPath(full_path.string(), root_.string())){
            Response response;
            {
                LoggingRequestHandle logger_(response);
                response = request_handle_utils::MakeStringResponse(http::status::bad_request, "BadRequest"sv, request.version(), 
                                                    request.keep_alive(), ContentType::TEXT);
            }
            return SendResponse(std::move(response), std::move(send));
        }
        //Обработка Api запросов
        else if(api::details::IsSubPath(target, "/api"s)){
            target = target.substr(target.find_first_of('/', 1));

            auto handler = [self = shared_from_this(), request, target, send] {
                Response response;
                {
                    LoggingRequestHandle logger_(response);
                    response = self->api_request_handler_.HandleRequest(request, target);

                }
                return self->SendResponse(std::move(response), std::move(send));
            };

            return boost::asio::dispatch(api_strand_, handler);
        }
        //Обработка запросов на получение файла
        else {
            if(request.method() != http::verb::get && request.method() != http::verb::head){
                return; 
            }

            Response response;
            {
                LoggingRequestHandle logger(response);                
                if(target == "/"){
                    target = "/index.html";
                }
                http::file_body::value_type file;
                fs::path full_path = root_.string() + target;

                if(boost::system::error_code ec; file.open(full_path.string().c_str(), beast::file_mode::read, ec), ec){
                    response = request_handle_utils::MakeStringResponse(http::status::not_found, "FileNotFound"sv, 
                                                        request.version(), request.keep_alive(), ContentType::TEXT);
                }
                else{
                    response = MakeFileResponse(http::status::ok, std::move(file), request.version(), 
                                                        request.keep_alive(), request_handle_utils::GetContentType(target));
                }
            }
            return SendResponse(std::move(response), std::move(send));
        }
    }

    void Tick(int delta){
        api_request_handler_.Tick(delta);
    }

private:
    FileResponse MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, 
                                      bool active_alive, std::string_view content_type);

    std::string DecodingURI(std::string_view uri);
    
    template <typename Send>
    void SendResponse(Response&& response, Send&& send){
        if(std::holds_alternative<StringResponse>(response)){
            send(std::get<StringResponse>(response));
            return;
        }
        send(std::get<FileResponse>(response));
    }
    
    api::ApiRequestHandler api_request_handler_;
    Strand api_strand_;
    fs::path root_;
};

}  // namespace http_handler
