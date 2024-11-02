#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    void SessionBase::Run() {
        net::dispatch(stream_.get_executor(),
                    beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                << logging::add_value(additional_data, log_data::MakeErrorData(ec.value(), ec.message(), "write"))
                                << "error";
            return;
        }

        if (close) {
            return Close();
        }

        Read();
    }

    void SessionBase::Read() {
        using namespace std::literals;
        request_ = {};
        stream_.expires_after(30s);
        http::async_read(stream_, buffer_, request_,
                         beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        using namespace std::literals;
        if (ec == http::error::end_of_stream) {
            return Close();
        }

        if (ec) {
            BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                << logging::add_value(additional_data, log_data::MakeErrorData(ec.value(), ec.message(), "read"))
                                << "error";
            return;
        }

        BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, boost::posix_time::second_clock::local_time())
                                << logging::add_value(additional_data, log_data::MakeRequestReceivedData(stream_.socket().remote_endpoint().address().to_string(), 
                                                      std::string(request_.target()), request_.method()))
                                << "request received";

        HandleRequest(std::move(request_));
    }

    void SessionBase::Close() {
        stream_.socket().shutdown(tcp::socket::shutdown_send);
    }
    
}  // namespace http_server