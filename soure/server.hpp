#include"db.hpp"
#include"matcher.hpp"
#include"online.hpp"
#include"room.hpp"
#include"session.hpp"
#include"util.hpp"

class gomoku_server
{
    websocketsvr _server;
    User_Table _ut;
    Online_Manager _om;
    Room_Manager _rm;
    Session_Manager _sm;
    Matcher_Queue_Manager _mqm;
    std::string _root_path;

    void http_resp(const websocketsvr::connection_ptr& con, bool result, const std::string& reason, websocketpp::http::status_code::value code)
    {
        Json::Value resp;
        resp["result"] = result;
        resp["reason"] = reason;
        std::string str;
        Json_Util::serialize(resp, str);
        con->set_body(str);
        con->set_status(code);
        con->append_header("Content-Type", "application/json");
    }

    void register_handle(const websocketsvr::connection_ptr& con)
    {
        websocketpp::config::asio::request_type req = con->get_request();
        std::string body = req.get_body();
        Json::Value reg_info;
        if(!Json_Util::unserialize(body, reg_info))
        {
            DBG_LOG("unserialize error.");
            http_resp(con, false, "request body error.", websocketpp::http::status_code::bad_request);
            return;
        }
        if(reg_info["name"].isNull() || reg_info["password"].isNull())
        {
            DBG_LOG("name or password can't be null.");
            http_resp(con, false, "name and password can't be null.", websocketpp::http::status_code::bad_request);
            return;
        }
        if(!_ut.insert(reg_info))
        {
            DBG_LOG("insert error.");
            http_resp(con, false, "name is already taken.", websocketpp::http::status_code::bad_request);
            return;
        }
        http_resp(con, true, "register success.", websocketpp::http::status_code::ok);
    }

    void login_handle(const websocketsvr::connection_ptr& con)
    {
        websocketpp::config::asio::request_type req = con->get_request();
        std::string body = req.get_body();
        Json::Value reg_info;
        if(!Json_Util::unserialize(body, reg_info))
        {
            DBG_LOG("unserialize error.");
            http_resp(con, false, "request body error.", websocketpp::http::status_code::bad_request);
            return;
        }
        if(reg_info["name"].isNull() || reg_info["password"].isNull())
        {
            DBG_LOG("name or password can't be null.");
            http_resp(con, false, "name and password can't be null.", websocketpp::http::status_code::bad_request);
            return;
        }
        if(!_ut.login(reg_info))
        {
            DBG_LOG("login error.");
            http_resp(con, false, "name or password error, login fail.", websocketpp::http::status_code::bad_request);
            return;
        }
        int uid = reg_info["id"].asInt();
        session_ptr sp = _sm.create_session(uid, LOGIN);
        if(sp.get() == nullptr)
        {
            DBG_LOG("create_session error.");
            http_resp(con, false, "login fail.", websocketpp::http::status_code::internal_server_error);
            return;
        }
        int sid = sp->get_sid();
        _sm.set_session_expire_time(sid, DEFAULT_TIMEOUT);
        std::string cookie_sid = "SID=" + std::to_string(sid);
        con->append_header("Set-Cookie", cookie_sid);
        http_resp(con, true, "login success.", websocketpp::http::status_code::ok);
    }

    bool get_cookie_value(std::string& cookie, const std::string& key, std::string& value)
    {
        std::vector<std::string> cookie_field;
        String_Util::str_split(cookie, "; ", cookie_field);
        for(auto& e : cookie_field)
        {
            std::vector<std::string> tmp_arr;
            String_Util::str_split(e, "=", tmp_arr);
            if(tmp_arr[0] == key) { value = tmp_arr[1]; return true;}
        }
        return false;
    }

    void info_handle(const websocketsvr::connection_ptr& con)
    {
        websocketpp::config::asio::request_type req = con->get_request();
        std::string cookie = req.get_header("Cookie");
        if(cookie.empty())
        {
            DBG_LOG("get_header error.");
            http_resp(con, false, "get cookie fail, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        std::string str_sid;
        if(!get_cookie_value(cookie, "SID", str_sid))
        {
            DBG_LOG("get_cookie_value error.");
            http_resp(con, false, "get sid in cookie fail, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        int sid = std::stoi(str_sid);
        session_ptr sp = _sm.get_session_by_sid(sid);
        if(sp.get() == nullptr)
        {
            DBG_LOG("get_session_by_sid error.");
            http_resp(con, false, "get session by sid fail, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        int uid = sp->get_uid();
        Json::Value user_info;
        if(!_ut.select_by_id(uid, user_info))
        {
            DBG_LOG("select_by_id error.");
            http_resp(con, false, "info with sid can't found, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        std::string str;
        Json_Util::serialize(user_info, str);
        con->set_body(str);
        con->set_status(websocketpp::http::status_code::ok);
        con->append_header("Content-Type", "application/json");
    }

    void file_handle(const websocketsvr::connection_ptr& con, const std::string& uri)
    {
        std::string path = _root_path + uri;
        if(uri == "/") path += "login.html";
        std::string buf;
        if(!File_Util::read_file(path, buf))
        {
            std::stringstream ss;
            ss << "<!doctype html><html><head>"
            << "<title>404 NOT FOUND</title><body>"
            << "<h1>404 NOT FOUND</h1>"
            << "</body></head></html>";
            con->set_body(ss.str());
            con->set_status(websocketpp::http::status_code::not_found);
            return;
        }
        con->set_body(buf);
        con->set_status(websocketpp::http::status_code::ok);
    }

    void handler_http(websocketpp::connection_hdl hdl)
    {
        std::cout << "handler http request." << std::endl;
        websocketsvr::connection_ptr con = _server.get_con_from_hdl(hdl);
        // std::cout << con->get_request_body() << std::endl;
        websocketpp::config::asio::request_type req = con->get_request();      
        std::cout << "method: " << req.get_method() << std::endl;
        std::cout << "uri: " << req.get_uri() << std::endl;
        std::cout << "body: " << req.get_body() << std::endl;
        std::cout << "cookie: " << req.get_header("Cookie") << std::endl;
        std::string method = req.get_method();
        std::string uri = req.get_uri();
        if(method == "POST", uri == "/reg")
        {
            register_handle(con);
        }
        else if(method == "POST", uri == "/login")
        {
            login_handle(con);
        }
        else if(method == "GET", uri == "/info")
        {
            info_handle(con);
        }
        else
        {
            file_handle(con, uri);
        }
    }

    void ws_handler_open(websocketpp::connection_hdl hdl)
    {

    }

    void ws_handler_close(websocketpp::connection_hdl hdl) {}

    void ws_handler_message(websocketpp::connection_hdl hdl, std::shared_ptr<websocketpp::config::core::message_type> msg) {}
public:
    gomoku_server(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag, const std::string& root_path):
        _ut(host, user, passwd, db, port, unix_socket, clientflag),
        _rm(&_om, &_ut),
        _sm(&_server),
        _mqm(&_ut, &_om, &_rm),
        _root_path(root_path)
    {
        _server.set_access_channels(websocketpp::log::alevel::none);
        _server.init_asio();
        _server.set_reuse_addr(true);
        _server.set_http_handler(std::bind(&gomoku_server::handler_http, this, std::placeholders::_1));
        _server.set_open_handler(std::bind(&gomoku_server::ws_handler_open, this, std::placeholders::_1));
        _server.set_close_handler(std::bind(&gomoku_server::ws_handler_close, this, std::placeholders::_1));
        _server.set_message_handler(std::bind(&gomoku_server::ws_handler_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void start()
    {
        _server.listen(9091);
        _server.start_accept();
        _server.run();
    }
};