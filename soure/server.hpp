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

    void hall_info_handle(const websocketsvr::connection_ptr& con)
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
        if(!_ut.select_by_uid(uid, user_info))
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

    void room_info_handle(const websocketsvr::connection_ptr& con)
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
        Room_Manager::room_ptr rp = _rm.get_room_by_uid(uid);
        if(rp.get() == nullptr)
        {
            DBG_LOG("get_room_by_uid error.");
            http_resp(con, false, "get room by uid fail, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        int black = rp->get_black();
        int white = rp->get_white();
        Json::Value white_info;
        Json::Value black_info;
        if(!_ut.select_by_uid(black, black_info))
        {
            DBG_LOG("select_by_id error.");
            http_resp(con, false, "info with black id can't found, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        if(!_ut.select_by_uid(white, white_info))
        {
            DBG_LOG("select_by_id error.");
            http_resp(con, false, "info with black id can't found, please login again.", websocketpp::http::status_code::bad_request);
            return;
        }
        Json::Value wb_info;
        wb_info["black_name"] = black_info["name"];
        wb_info["black_id"] = black_info["id"];
        wb_info["black_score"] = black_info["score"];
        wb_info["black_total_games"] = black_info["total_games"];
        wb_info["black_win_games"] = black_info["win_games"];

        wb_info["white_name"] = white_info["name"];
        wb_info["white_id"] = white_info["id"];
        wb_info["white_score"] = white_info["score"];
        wb_info["white_total_games"] = white_info["total_games"];
        wb_info["white_win_games"] = white_info["win_games"];

        wb_info["uid"] = uid;
        std::string str;
        Json_Util::serialize(wb_info, str);
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

    void handle_http(websocketpp::connection_hdl hdl)
    {
        INF_LOG("handler http request.");
        websocketsvr::connection_ptr con = _server.get_con_from_hdl(hdl);
        websocketpp::config::asio::request_type req = con->get_request();
        INF_LOG("method: %s", req.get_method().c_str());
        INF_LOG("uri: %s", req.get_uri().c_str());
        INF_LOG("body: %s", req.get_body().c_str());
        INF_LOG("cookie: %s", req.get_header("Cookie").c_str());
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
        else if(method == "GET", uri == "/hall/info")
        {
            hall_info_handle(con);
        }
        else if(method == "GET", uri == "/room/info")
        {
            INF_LOG("room_info");
            room_info_handle(con);
        }
        else
        {
            file_handle(con, uri);
        }
    }

    void ws_resp(const websocketsvr::connection_ptr& con, Json::Value& resp)
    {
        std::string body;
        Json_Util::serialize(resp, body);
        // INF_LOG(body.c_str());
        con->send(body);
    }

    session_ptr get_session_by_cookie(const websocketsvr::connection_ptr& con, const std::string& optype)
    {
        websocketpp::config::asio::request_type req = con->get_request();
        std::string cookie = req.get_header("Cookie");
        if(cookie.empty()) // 取出cookie，没有就是错误请求
        {
            DBG_LOG("get_header error.");
            Json::Value resp;
            resp["optype"] = optype;
            resp["result"] = false;
            resp["reason"] = "get cookie fail, please login again.";
            ws_resp(con, resp);
            return session_ptr();
        }
        std::string str_sid;
        if(!get_cookie_value(cookie, "SID", str_sid)) // 获取cookie中的sid，没有就是错误cookie
        {
            DBG_LOG("get_cookie_value error.");
            Json::Value resp;
            resp["optype"] = optype;
            resp["result"] = false;
            resp["reason"] = "get sid in cookie fail, please login again.";
            ws_resp(con, resp);
            return session_ptr();
        }
        int sid = std::stoi(str_sid);
        session_ptr sp = _sm.get_session_by_sid(sid);
        if(sp.get() == nullptr) // 根据sid获取session，没有就是session超时销毁了，登陆失效，重新登录
        {
            DBG_LOG("get_session_by_sid error.");
            Json::Value resp;
            resp["optype"] = optype;
            resp["result"] = false;
            resp["reason"] = "get session by sid fail, please login again.";
            ws_resp(con, resp);
            return session_ptr();
        }
        return sp;
    }

    void ws_open_hall(websocketsvr::connection_ptr& con)
    {
        session_ptr sp = get_session_by_cookie(con, "hall_ready");
        if(sp.get() == nullptr) return;
        if(_om.is_in_hall(sp->get_uid()) || _om.is_in_room(sp->get_uid()))
        {
            DBG_LOG("repeated login.");
            Json::Value resp;
            resp["optype"] = "hall_ready";
            resp["result"] = false;
            resp["reason"] = "repeated login!";
            return ws_resp(con, resp);
        }
        _om.enter_hall(sp->get_uid(), con);
        Json::Value resp;
        resp["optype"] = "hall_ready";
        resp["result"] = true;
        ws_resp(con, resp);
        _sm.set_session_expire_time(sp->get_sid(), SESSION_FOREVER);
    }

    void ws_open_room(websocketsvr::connection_ptr& con)
    {
        session_ptr sp = get_session_by_cookie(con, "hall_ready");
        if(sp.get() == nullptr) return;
        Json::Value resp;
        int uid = sp->get_uid();
        if(_om.is_in_hall(uid) || _om.is_in_room(uid))
        {
            DBG_LOG("repeated login.");
            resp["optype"] = "room_ready";
            resp["result"] = false;
            resp["reason"] = "repeated login!";
            return ws_resp(con, resp);
        }
        Room_Manager::room_ptr rp = _rm.get_room_by_uid(uid);
        if(rp.get() == nullptr)
        {
            DBG_LOG("room not found.");
            resp["optype"] = "room_ready";
            resp["result"] = false;
            resp["reason"] = "room not found.";
            return ws_resp(con, resp);
        }
        _om.enter_room(uid, con);
        _sm.set_session_expire_time(sp->get_sid(), SESSION_FOREVER);
        resp["optype"] = "room_ready";
        resp["result"] = true;
        resp["room_id"] = rp->get_room_id();
        resp["uid"] = uid;
        resp["white_id"] = rp->get_white();
        resp["black_id"] = rp->get_black();
        return ws_resp(con, resp);
    }

    void ws_handle_open(websocketpp::connection_hdl hdl)
    {
        INF_LOG("open websocket.");
        websocketsvr::connection_ptr con = _server.get_con_from_hdl(hdl);
        websocketpp::config::asio::request_type req = con->get_request();
        // std::string method = req.get_method();
        std::string uri = req.get_uri();
        if(uri == "/hall")
        {
            ws_open_hall(con);
        }
        else if(uri == "/room")
        {
            ws_open_room(con);
        }
    }

    void ws_close_hall(const websocketsvr::connection_ptr& con)
    {
        session_ptr sp = get_session_by_cookie(con, "hall_close");
        if(sp.get() == nullptr) return;
        _om.exit_hall(sp->get_uid());
        _sm.set_session_expire_time(sp->get_sid(), DEFAULT_TIMEOUT);
    }

    void ws_close_room(const websocketsvr::connection_ptr& con)
    {
        session_ptr sp = get_session_by_cookie(con, "hall_close");
        if(sp.get() == nullptr) return;
        _om.exit_room(sp->get_uid());
        _sm.set_session_expire_time(sp->get_sid(), DEFAULT_TIMEOUT);
        _rm.remove_user_room(sp->get_uid());
    }

    void ws_handle_close(websocketpp::connection_hdl hdl)
    {
        INF_LOG("close websocket.");
        websocketsvr::connection_ptr con = _server.get_con_from_hdl(hdl);
        websocketpp::config::asio::request_type req = con->get_request();
        std::string uri = req.get_uri();
        if(uri == "/hall")
        {
            return ws_close_hall(con);
        }
        else if(uri == "/room")
        {
            return ws_close_room(con);
        }
    }

    void ws_message_hall(const websocketsvr::connection_ptr& con, std::shared_ptr<websocketpp::config::core::message_type> msg)
    {
        session_ptr sp = get_session_by_cookie(con, "handle_request");
        if(sp.get() == nullptr) return;
        std::string req = msg->get_payload();
        Json::Value req_json;
        Json::Value resp;
        if(!Json_Util::unserialize(req, req_json))
        {
            DBG_LOG("unserialize error.");
            resp["optype"] = "handle_request";
            resp["result"] = false;
            resp["reason"] = "bad request.";
            return ws_resp(con, resp);
        }
        if(!req_json["optype"].isNull() && req_json["optype"].asString() == "match_start")
        {
            if(!_mqm.add(sp->get_uid())) // 一般来说就不可能错误，session创建必查数据库，出错了就不是客户端的问题
            {
                DBG_LOG("mqm::add error.");
                return;
            }
            resp["optype"] = "match_start";
            resp["result"] = true;
        }
        else if(!req_json["optype"].isNull() && req_json["optype"].asString() == "match_stop")
        {
            if(!_mqm.del(sp->get_uid()))
            {
                DBG_LOG("mqm::del error.");
                return;
            }
            resp["optype"] = "match_stop";
            resp["result"] = true;
        }
        else
        {
            DBG_LOG("unknow optype.");
            resp["optype"] = "unknow";
            resp["result"] = false;
            resp["reason"] = "unknow optype.";
        }
        return ws_resp(con, resp);
    }

    void ws_message_room(const websocketsvr::connection_ptr& con, std::shared_ptr<websocketpp::config::core::message_type> msg)
    {
        session_ptr sp = get_session_by_cookie(con, "handle_request");
        if(sp.get() == nullptr) return;
        Json::Value resp;
        int uid = sp->get_uid();
        Room_Manager::room_ptr rp = _rm.get_room_by_uid(uid);
        if(rp.get() == nullptr)
        {
            resp["optype"] = "handle_request";
            resp["result"] = false;
            resp["reason"] = "room info not found.";
            return ws_resp(con, resp);
        }
        std::string req = msg->get_payload();
        Json::Value req_json;
        if(!Json_Util::unserialize(req, req_json))
        {
            DBG_LOG("unserialize error.");
            resp["optype"] = "handle_request";
            resp["result"] = false;
            resp["reason"] = "bad request.";
            return ws_resp(con, resp);
        }
        return rp->handle_request(req_json);
    }

    void ws_handle_message(websocketpp::connection_hdl hdl, std::shared_ptr<websocketpp::config::core::message_type> msg)
    {
        INF_LOG("handle websocket request.");
        websocketsvr::connection_ptr con = _server.get_con_from_hdl(hdl);
        websocketpp::config::asio::request_type req = con->get_request();
        std::string uri = req.get_uri();
        if(uri == "/hall")
        {
            return ws_message_hall(con, msg);
        }
        else
        {
            return ws_message_room(con, msg);
        }
    }
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
        _server.set_http_handler(std::bind(&gomoku_server::handle_http, this, std::placeholders::_1));
        _server.set_open_handler(std::bind(&gomoku_server::ws_handle_open, this, std::placeholders::_1));
        _server.set_close_handler(std::bind(&gomoku_server::ws_handle_close, this, std::placeholders::_1));
        _server.set_message_handler(std::bind(&gomoku_server::ws_handle_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void start()
    {
        _server.listen(9091);
        _server.start_accept();
        _server.run();
    }
};