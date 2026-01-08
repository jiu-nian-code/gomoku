#pragma once
#include"websocketpp/server.hpp"
#include"websocketpp/config/asio_no_tls.hpp"
#include<unordered_map>
#include<mutex>
#include"util.hpp"
#include"db.hpp"

typedef websocketpp::server<websocketpp::config::asio> websocketsvr;

class Online_Manager
{
    std::unordered_map<int, websocketsvr::connection_ptr> _game_room;
    std::unordered_map<int, websocketsvr::connection_ptr> _game_hall;
    std::unordered_map<int, websocketsvr::connection_ptr> _review_room;
    std::mutex _mt;
public:
    void enter_room(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_room[uid] = con;
    }

    void enter_hall(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_hall[uid] = con;
    }

    void enter_review_room(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _review_room[uid] = con;
    }

    void exit_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_room.erase(uid); // 没有返回0，不会报错
    }

    void exit_hall(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_hall.erase(uid);
    }

    void exit_review_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _review_room.erase(uid);
    }

    bool is_in_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_room.find(uid);
        if(it == _game_room.end()) return false;
        return true;
    }

    bool is_in_hall(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_hall.find(uid);
        if(it == _game_hall.end()) return false;
        return true;
    }

    bool is_in_review_room(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _review_room.find(uid);
        if(it == _review_room.end()) return false;
        return true;
    }

    bool get_conn_from_room(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_room.find(uid);
        if(it == _game_room.end())
        {
            DBG_LOG("the user of this uid is not exists in room.");
            return false;
        }
        con = it->second;
        return true;
    }

    bool get_conn_from_hall(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_hall.find(uid);
        if(it == _game_hall.end())
        {
            DBG_LOG("the user of this uid is not exists in hall.");
            return false;
        }
        con = it->second;
        return true;
    }

    bool get_conn_from_review_room(int uid, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _review_room.find(uid);
        if(it == _review_room.end())
        {
            DBG_LOG("the user of this uid is not exists in review_room.");
            return false;
        }
        con = it->second;
        return true;
    }
};