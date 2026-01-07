#include"db.hpp"
#include"matcher.hpp"
#include"online.hpp"
#include"room.hpp"
#include"session.hpp"
#include"util.hpp"
#include<functional>
#include<queue>

#define RCONNECTION_TIME 10000

struct Task
{
    using task_func = std::function<bool()>;
    task_func _tf = nullptr;
    bool _is_cancel = false;
    Task(const task_func& tf) : _tf(tf) {}
    ~Task() 
    {
        if(!_is_cancel && _tf)
        {
            std::cout << "task执行" << std::endl;
            _tf();
        }
        else std::cout << "task取消执行" << std::endl;
    }
};

class Gomoku_Server
{
    websocketsvr _server;
    Matches_Table _mtable;
    User_Table _ut;
    Online_Manager _om;
    Room_Manager _rm;
    Session_Manager _sm;
    Matcher_Queue_Manager _mqm;
    std::string _root_path;
    std::mutex _mt;

    using task_ptr = std::shared_ptr<Task>;
    std::unordered_map<int, task_ptr> _uid_task; // 这里用uid做任务id
    std::unordered_map<int, websocketsvr::timer_ptr> _uid_time;

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
        if(cancel_remove_task(uid)) rp->board_reset(resp);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 以下函数有线程安全，但是只在ws_close_room中执行，所以ws_close_room统一加锁

    void remove_task(int uid)
    {
        // _uid_task.erase(uid);
        // _uid_time.erase(uid);
        task_ptr to_be_deleted; // 在锁外声明，先取再执行，防止死锁
        {
            std::unique_lock<std::mutex> lock(_mt);
            auto it = _uid_task.find(uid);
            if (it != _uid_task.end())
            {
                to_be_deleted = it->second; // 拷贝指针，增加引用计数
                _uid_task.erase(it);
            }
            _uid_time.erase(uid);
        }
    }

    void append_task(int uid, task_ptr& taskptr)
    {
        _uid_task.insert(std::make_pair(uid, taskptr));
        _uid_time.insert(std::make_pair(uid, _server.set_timer(RCONNECTION_TIME, std::bind(&Gomoku_Server::remove_task, this, uid))));
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool cancel_remove_task(int uid) // 该函数在ms_open_room中执行，所以单独加锁
    {
        std::unique_lock<std::mutex> lock(_mt);
        if(_uid_task.count(uid) == 0) return false;
        _uid_task[uid]->_is_cancel = true;
        std::cout << "cancel_remove_task" << std::endl;
        return true;
    }

    bool is_close_room_now(int uid, const Room_Manager::room_ptr& rp) // 如果对面已经退出了，设置了倒计时退出，那么就应该立即执行那个人的退出任务，然后执行自己的，没有超时重连的可能性了
    {
        int black = rp->get_black();
        int white = rp->get_white();
        int oppsite = uid == white ? black : white;
        if(((_uid_task.count(oppsite) == 0 && _uid_time.count(oppsite) == 0) ||
        (_uid_task[oppsite]->_is_cancel)) && _om.is_in_room(oppsite)) return false;
        if(_uid_time.count(oppsite) == 0) // 一般情况绝对不会走进去，走进去就是线程不安全
        {
            ERR_LOG("thread is not safe!");
            exit(-1);
        }
        _uid_time[oppsite]->cancel();
        // _uid_time.erase(oppsite);
        return true;
    }

    void ws_close_room(const websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        session_ptr sp = get_session_by_cookie(con, "hall_close");
        if(sp.get() == nullptr) return;
        int uid = sp->get_uid();
        _om.exit_room(uid);
        _sm.set_session_expire_time(sp->get_sid(), DEFAULT_TIMEOUT);
        Room_Manager::room_ptr rp = _rm.get_room_by_uid(uid);
        if(rp.get() == nullptr) return;
        if(!is_close_room_now(uid, rp))
        {
            // std::cout << 1 << std::endl;
            task_ptr taskptr(new Task(std::bind(&Room_Manager::remove_user_room, &_rm, uid)));
            if(_uid_task.count(uid) != 0 && _uid_time.count(uid) != 0 && _uid_task[uid]->_is_cancel) // 看看之前map中有没有连接对象
            {
                _uid_time[uid]->cancel();
                _server.set_timer(0, std::bind(&Gomoku_Server::append_task, this, uid, taskptr));
            }
            else append_task(uid, taskptr);
        }
        else
        {
            // std::cout << 2 << std::endl;
            // 对面已经退出，cancel取消了，但是cancel不是立即执行，所以将自己也压进任务队列，然后0秒执行，排在cancel后面，确保根据退出顺序结算输赢得分
            task_ptr taskptr(new Task(std::bind(&Room_Manager::remove_user_room, &_rm, uid)));
            _uid_task.insert(std::make_pair(uid, taskptr));
            _server.set_timer(0, std::bind(&Gomoku_Server::remove_task, this, uid));
        }
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
        else if(!req_json["optype"].isNull() && req_json["optype"].asString() == "get_matches")
        {
            resp["optype"] = "get_matches";
            _mtable.select_by_uid(req_json["uid"].asInt(), resp);
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
        
        if(req_json["optype"].asString() == "exit") // 只有按下退出按钮时才会有这个optype，这时直接0秒延迟退出房间决出胜负
        {
            std::unique_lock<std::mutex> lock(_mt);
            rp->check_req(req_json);
            _om.exit_room(uid);
            _sm.set_session_expire_time(sp->get_sid(), DEFAULT_TIMEOUT);
            Room_Manager::room_ptr rp = _rm.get_room_by_uid(uid);
            if(rp.get() == nullptr) return; // 同close函数一样，房间都没了就别搞什么删除了

            task_ptr taskptr(new Task(std::bind(&Room_Manager::remove_user_room, &_rm, uid)));
            _uid_task.insert(std::make_pair(uid, taskptr));
            _server.set_timer(0, std::bind(&Gomoku_Server::remove_task, this, uid));
        }
        else
        {
            int ret = rp->handle_request(req_json);
            if(ret != 0)
            {
                std::unique_lock<std::mutex> lock(_mt);
                int white_id = rp->get_black();
                int black_id = rp->get_white();
                int cur_id = ret == 1 ? black_id : white_id;
                _om.exit_room(cur_id);
                _sm.set_session_expire_time(sp->get_sid(), DEFAULT_TIMEOUT);
                Room_Manager::room_ptr rp = _rm.get_room_by_uid(cur_id);
                if(rp.get() == nullptr) return;
                if(!is_close_room_now(cur_id, rp))
                {
                    // std::cout << 1 << std::endl;
                    task_ptr taskptr(new Task(std::bind(&Room_Manager::remove_user_room, &_rm, cur_id)));
                    if(_uid_task.count(cur_id) != 0 && _uid_time.count(cur_id) != 0 && _uid_task[cur_id]->_is_cancel) // 看看之前map中有没有连接对象
                    {
                        _uid_time[cur_id]->cancel();
                        _server.set_timer(0, std::bind(&Gomoku_Server::append_task, this, cur_id, taskptr));
                    }
                    else append_task(cur_id, taskptr);
                }
                else
                {
                    // std::cout << 2 << std::endl;
                    // 对面已经退出，cancel取消了，但是cancel不是立即执行，所以将自己也压进任务队列，然后0秒执行，排在cancel后面，确保根据退出顺序结算输赢得分
                    task_ptr taskptr(new Task(std::bind(&Room_Manager::remove_user_room, &_rm, cur_id)));
                    _uid_task.insert(std::make_pair(cur_id, taskptr));
                    _server.set_timer(0, std::bind(&Gomoku_Server::remove_task, this, cur_id));
                }
            }
        }
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
    Gomoku_Server(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag, const std::string& root_path):
        _mtable(host, user, passwd, db, port, unix_socket, clientflag),
        _ut(host, user, passwd, db, port, unix_socket, clientflag),
        _rm(&_om, &_ut, &_mtable),
        _sm(&_server),
        _mqm(&_ut, &_om, &_rm),
        _root_path(root_path)
    {
        _server.set_access_channels(websocketpp::log::alevel::none);
        _server.init_asio();
        _server.set_reuse_addr(true);
        _server.set_http_handler(std::bind(&Gomoku_Server::handle_http, this, std::placeholders::_1));
        _server.set_open_handler(std::bind(&Gomoku_Server::ws_handle_open, this, std::placeholders::_1));
        _server.set_close_handler(std::bind(&Gomoku_Server::ws_handle_close, this, std::placeholders::_1));
        _server.set_message_handler(std::bind(&Gomoku_Server::ws_handle_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void start()
    {
        _server.listen(9091);
        _server.start_accept();
        _server.run();
    }
};