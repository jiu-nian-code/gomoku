#include<iostream>

#include<websocketpp/config/asio_no_tls.hpp>

#include<websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> websocketsvr;

void PRINT()
{
    std::cout << "cnm" << std::endl;
}

void handler_http(websocketsvr* server, websocketpp::connection_hdl hdl)
{
    std::cout << "handler http request." << std::endl;
    websocketsvr::connection_ptr con = server->get_con_from_hdl(hdl);
    // std::cout << con->get_request_body() << std::endl;
    websocketpp::config::asio::request_type req = con->get_request();
    std::cout << "method: " << req.get_method() << std::endl;
    std::cout << "uri: " << req.get_uri() << std::endl;
    std::cout << "body: " << req.get_body() << std::endl;
    std::stringstream ss;
    ss << "<!doctype html><html><head>"
    << "<title>hello websocket</title><body>"
    << "<h1>hello websocketpp</h1>"
    << "</body></head></html>";
    con->set_body(ss.str());
    con->set_status(websocketpp::http::status_code::ok);
    websocketsvr::timer_ptr tp = server->set_timer(5000, std::bind(PRINT));
    tp->cancel();
}

void handler_open(websocketpp::connection_hdl hdl)
{
    std::cout << "get a new connect" << std::endl;
}

void handler_close(websocketpp::connection_hdl hdl)
{
    std::cout << "close connect" << std::endl;
}

void handler_message(websocketsvr* server, websocketpp::connection_hdl hdl, std::shared_ptr<websocketpp::config::core::message_type> msg)
{
    std::cout << "client say: " << msg->get_payload() << std::endl;
    server->send(hdl, msg->get_payload(), websocketpp::frame::opcode::text);
}

int main()
{
    websocketsvr server;
    server.set_access_channels(websocketpp::log::alevel::none);
    server.init_asio();
    server.set_reuse_addr(true);
    server.set_http_handler(std::bind(&handler_http, &server, std::placeholders::_1));
    server.set_open_handler(handler_open);
    server.set_close_handler(handler_close);
    server.set_message_handler(std::bind(&handler_message, &server, std::placeholders::_1, std::placeholders::_2));
    server.listen(9091);
    server.start_accept();
    server.run();
    return 0;
}