#include<iostream>

#include<websocketpp/config/asio_no_tls.hpp>

#include<websocketpp/server.hpp>

// typedef websocketpp::server<websocketpp::config::asio> websocketsvr;

void handler_http(websocketpp::connection_hdl hdl)
{

}

void handler_open(websocketpp::connection_hdl hdl)
{

}

void handler_close(websocketpp::connection_hdl hdl)
{

}

void handler_message(websocketpp::connection_hdl hdl, std::shared_ptr<websocketpp::config::core::message_type> ptr)
{

}

int main()
{
    websocketpp::server<websocketpp::config::asio> server;
    server.set_access_channels(websocketpp::log::alevel::none);
    server.init_asio();
    server.set_http_handler(handler_http);
    server.set_open_handler(handler_open);
    server.set_close_handler(handler_close);
    server.set_message_handler(handler_message);
    return 0;
}