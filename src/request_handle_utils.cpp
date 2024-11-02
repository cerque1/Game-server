#include "request_handle_utils.h"

#include <boost/json.hpp>

namespace request_handle_utils{

namespace json = boost::json;
using value_type = json::object::value_type;

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
                                    bool active_alive, std::string_view content_type, std::string_view allow){
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.keep_alive(active_alive);
    response.content_length(body.size());
    if(!allow.empty()){
        response.set(http::field::allow, allow);
    }
    response.set(http::field::cache_control, "no-cache");
    return response;
}

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
                                    bool active_alive, std::string_view content_type){
    return MakeStringResponse(status, body, http_version, active_alive, content_type, "");
}

std::string MakeErrorMessage(const std::string& code, const std::string& message){
    json::object error_message;

    error_message.insert(value_type("code", code));
    error_message.insert(value_type("message", message));

    return json::serialize(error_message);
}

std::string_view GetContentType(std::string_view file){
    std::string extension(file.substr(file.find_last_of('.') + 1));

    for(char& c : extension){
        c = tolower(c);
    }

    if(extension == "htm" || extension == "html") return ContentType::TEXT_HTML;
    else if(extension == "css") return ContentType::TEXT_CSS;
    else if(extension == "txt") return ContentType::TEXT;
    else if(extension == "js") return ContentType::TEXT_JS;
    else if(extension == "json") return ContentType::APPLICATION_JSON;
    else if(extension == "xml") return ContentType::APPLICATION_XML;
    else if(extension == "png") return ContentType::IMAGE_PNG;
    else if(extension == "jpg" || extension == "jpe" || extension == "jpeg") return ContentType::IMAGE_JPEG;
    else if(extension == "gif") return ContentType::IMAGE_GIF;
    else if(extension == "bmp") return ContentType::IMAGE_BMP;
    else if(extension == "ico") return ContentType::IMAGE_ICO;
    else if(extension == "tiff" || extension == "tif") return ContentType::IMAGE_TIFF;
    else if(extension == "svg" || extension == "svgz") return ContentType::IMAGE_SVG_XML;
    else if(extension == "mp3") return ContentType::AUDIO_MP3;
    else return ContentType::APPLICATION_OCTET_STREAM;
}

}