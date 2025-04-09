#include <boost/json.hpp>

#include "request_handler.h"

namespace http_handler {

namespace json = boost::json;
using value_type = json::object::value_type;

namespace details{

    int HexToDec(std::string num){
        unsigned dec;
        sscanf(num.c_str(), "%x", &dec);
        return dec;
    }

}   //details

void LoggingRequestHandle::LogResponse(){
    if(std::holds_alternative<StringResponse>(response_)){
        OnLogResponse(std::get<StringResponse>(response_));
        return;
    }
    OnLogResponse(std::get<FileResponse>(response_));
}

FileResponse RequestHandler::MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, 
                                      bool active_alive, std::string_view content_type){
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = std::move(body);
    response.keep_alive(active_alive);
    response.content_length(body.size());
    response.prepare_payload();
    return response;
}

std::string RequestHandler::DecodingURI(std::string_view uri){
    std::string decoding_uri;
    
    for(size_t i = 0; i < uri.size(); i++){ 
        if(uri[i] == '%'){
            std::string hex;
            hex = static_cast<char>(uri[++i]);
            hex += static_cast<char>(uri[++i]);
            decoding_uri += static_cast<char>(details::HexToDec(hex));
        }
        else if(uri[i] == '+'){
            decoding_uri += ' ';
        }
        else{
            decoding_uri += uri[i];
        }
    }

    return decoding_uri;
}

}   // namespace http_handler
