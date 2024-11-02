#pragma once
#include <string_view>

#include <boost/beast/http.hpp>

using namespace std::literals;

namespace request_handle_utils{

namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

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

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
                                      bool active_alive, std::string_view content_type);

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
                                      bool active_alive, std::string_view content_type, std::string_view allow);

std::string MakeErrorMessage(const std::string& code, const std::string& message);

std::string_view GetContentType(std::string_view file);
} // request_handle_utils                                     