#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/beast/http.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace log_data{
    using namespace std::literals;

    boost::json::value MakeStartServerData(int port, boost::asio::ip::address addr);
    boost::json::value MakeRequestReceivedData(const std::string& ip, const std::string& URI, boost::beast::http::verb method);
    boost::json::value MakeResponseSendData(int response_time, boost::beast::http::status status, const std::string& content_type);
    boost::json::value MakeServerExitedData(int code, const std::string& expretion = ""s);
    boost::json::value MakeErrorData(int code, const std::string& text, const std::string& where);

} //log_data

namespace formatter{

    void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

} //formatter
