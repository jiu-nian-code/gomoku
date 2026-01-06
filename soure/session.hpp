#pragma once
#include"online.hpp"
#define DEFAULT_TIMEOUT 30000
#define SESSION_FOREVER -1

typedef enum {UNLOGIN, LOGIN} ses_stu;
class Session
{
    int _session_id;
    int _uid;
    ses_stu _ss;
    websocketsvr::timer_ptr _tp;
public:
    Session(int session_id) : _session_id(session_id), _uid(-1), _ss(UNLOGIN) { DBG_LOG("session %p create successful.", this); }
    ~Session() { DBG_LOG("session %p destory.", this); }
    int get_sid() { return _session_id; }
    void set_uid(int uid) { _uid = uid; }
    int get_uid() { return _uid; }
    void set_status(ses_stu ss) { _ss = ss; }
    bool is_login() { return _ss == LOGIN; }
    void set_timer_ptr(const websocketsvr::timer_ptr& tp) { _tp = tp; }
    websocketsvr::timer_ptr& get_timer_ptr() { return _tp; }
};

using session_ptr = std::shared_ptr<Session>;
class Session_Manager
{
    static int _next_sid;
    std::mutex _mt;
    websocketsvr* _server;
    std::unordered_map<int, session_ptr> _session;
public:
    Session_Manager(websocketsvr* server) : _server(server) {}
    session_ptr create_session(int uid, ses_stu ss)
    {
        std::unique_lock<std::mutex> lock(_mt);
        session_ptr sp(new Session(_next_sid));
        sp->set_uid(uid);
        sp->set_status(ss);
        _session.insert(std::make_pair(_next_sid, sp));
        ++_next_sid;
        return sp;
    }
    
    session_ptr get_session_by_sid(int sid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _session.find(sid);
        if(it == _session.end()) return session_ptr();
        return it->second;
    }

    void session_append(session_ptr sp)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _session.insert(std::make_pair(sp->get_sid(), sp));
    }

    void remove_session(int sid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        // std::cout << "remove_session" << std::endl;
        _session.erase(sid);
    }

    void set_session_expire_time(int sid, int timeout)
    {
        session_ptr sp = _session[sid];
        if(sp.get() == nullptr) return;

        websocketsvr::timer_ptr tp = sp->get_timer_ptr();
        if(tp.get() == nullptr && timeout < 0) // 永久转永久，-1表示不定时，而是永久存在
        {
            return; // session的time_ptr为空就表示改session就已经是永久的了，这时什么都不用做
        }
        else if(tp.get() == nullptr && timeout >= 0) // 永久转定时销毁
        {
            // std::unique_lock<std::mutex> lock(_mt);
            websocketsvr::timer_ptr new_tp = _server->set_timer(timeout, std::bind(&Session_Manager::remove_session, this, sid));
            sp->set_timer_ptr(new_tp);
        }
        else if(tp.get() != nullptr && timeout < 0) // 定时销毁转永久
        {
            // std::unique_lock<std::mutex> lock(_mt); 是否要加锁？TODO
            tp->cancel();
            sp->set_timer_ptr(websocketsvr::timer_ptr());
            _server->set_timer(0, std::bind(&Session_Manager::session_append, this, sp));
        }
        else if(tp.get() != nullptr && timeout >= 0)// 定时销毁重置
        {
            // std::unique_lock<std::mutex> lock(_mt); 是否要加锁？TODO
            tp->cancel();
            sp->set_timer_ptr(websocketsvr::timer_ptr());
            _server->set_timer(0, std::bind(&Session_Manager::session_append, this, sp));
            websocketsvr::timer_ptr new_tp = _server->set_timer(timeout, std::bind(&Session_Manager::remove_session, this, sid));
            sp->set_timer_ptr(new_tp); // 后两步绝对不能和第二步交换，如果新设置的就是0秒后销毁，那么可能会出现cancel先执行了，然后设置set，然后继续创建，这样remove后又创建，session永久存在了
        }
    }
    
    // bool is_in_session(int sid) // for debug
    // {
    //     auto it = _session.find(sid);
    //     if(it == _session.end()) return false;
    //     return true;
    // }
};

int Session_Manager::_next_sid = 0;