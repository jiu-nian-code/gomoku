#include"online.hpp"

typedef enum {UNLOG, LOGIN} ses_stu;
class Session
{
    int _session_id;
    int _uid;
    ses_stu _ss;
    websocketsvr::timer_ptr _tp;
public:
    Session(int session_id) : _session_id(session_id), _uid(-1), _ss(UNLOG) { DBG_LOG("session %p create successful.", this); }
    ~Session() { DBG_LOG("session %p destory.", this); }
    void set_uid(int uid) { _uid = uid; }
    int get_uid() { return _uid; }
    void set_status(ses_stu ss) { _ss = ss; }
    bool is_login() { return _ss == LOGIN; }
    void set_timer_ptr(websocketsvr::timer_ptr& tp) { _tp = tp; }
    websocketsvr::timer_ptr& get_timer_ptr() { return _tp; }
};

using session_ptr = std::shared_ptr<Session>;
class Session_Manager
{
    static int _next_sid;
    std::mutex _mt;
    websocketsvr* _wss;
    std::unordered_map<int, session_ptr> _session;
public:
    Session_Manager() {}
    session_ptr create_session(int uid, ses_stu ss) {}
    session_ptr get_session_by_id(int id) {}
    void remove_session(int id) {}
    void set_session_expire_time() {}
};

int Session_Manager::_next_sid = 0;