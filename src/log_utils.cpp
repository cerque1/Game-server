#include "log_utils.h"

namespace log_data{

    boost::json::value MakeStartServerData(int port, boost::asio::ip::address addr){
        boost::json::object answer;
        answer.insert(boost::json::object::value_type{"port", port});
        answer.insert(boost::json::object::value_type{"address", addr.to_string()});
        return answer;
    }

    boost::json::value MakeRequestReceivedData(const std::string& ip, const std::string& URI, boost::beast::http::verb method){
        boost::json::object answer;
        answer.insert(boost::json::object::value_type{"ip", ip});
        answer.insert(boost::json::object::value_type{"URI", URI});
        answer.insert(boost::json::object::value_type{"method", std::string(boost::beast::http::to_string(method))});
        return answer;
    }

    boost::json::value MakeResponseSendData(int response_time, boost::beast::http::status status, const std::string& content_type){
        boost::json::object answer;
        answer.insert(boost::json::object::value_type{"response_time", response_time});
        answer.insert(boost::json::object::value_type{"code", static_cast<int>(status)});
        answer.insert(boost::json::object::value_type{"content_type", content_type});
        return answer;
    }

    boost::json::value MakeServerExitedData(int code, const std::string& expretion){
        boost::json::object answer;
        answer.insert(boost::json::object::value_type{"code", code});
        if(!expretion.empty()){
            answer.insert(boost::json::object::value_type{"expretion", expretion});
        }
        return answer;
    }

    boost::json::value MakeErrorData(int code, const std::string& text, const std::string& where){
        boost::json::object answer;
        answer.insert(boost::json::object::value_type{"code", code});
        answer.insert(boost::json::object::value_type{"text", text});
        answer.insert(boost::json::object::value_type{"where", where});
        return answer;
    }
}

namespace formatter{
    void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
        strm << boost::json::serialize(boost::json::object{
            {"timestamp", to_iso_extended_string(*rec[timestamp])},
            {"data", *rec[additional_data]},
            {"message", boost::json::string{*rec[logging::expressions::smessage]}}
            });
    }
}
