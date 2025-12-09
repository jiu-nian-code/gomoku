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
    std::mutex _mt;
public:
    void enter_room(int id, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_room[id] = con;
    }

    void enter_hall(int id, websocketsvr::connection_ptr& con)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_hall[id] = con;
    }

    void exit_room(int id)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_room.erase(id);
    }

    void exit_hall(int id)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _game_hall.erase(id);
    }

    bool is_in_room(int id)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_room.find(id);
        if(it == _game_room.end())
        {
            DBG_LOG("the user of this id is not exists in room.");
            return false;
        }
        return true;
    }

    bool is_in_hall(int id)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _game_hall.find(id);
        if(it == _game_hall.end())
        {
            DBG_LOG("the user of this id is not exists in hall.");
            return false;
        }
        return true;
    }
};

typedef enum { GAME_START, GAME_OVER } Game_Stu;
class Room
{
    int _room_id; // 房间id
    int _user_count; // 房间中的用户数量
    int _white_id; // 白棋方用户id
    int _black_id; // 黑棋方用户id
    Online_Manager* _om; // 用户管理类
    User_Table* _ut;
    Game_Stu _gs;
    std::vector<std::vector<int>> _chessboard; // 棋盘
public:
    Room(int room_id, Online_Manager* om, User_Table* ut) : 
        _room_id(room_id), _user_count(0),
        _white_id(-1), _black_id(-1),
        _om(om), _ut(ut), _gs(GAME_START)
    {
        DBG_LOG("%d room create success.");
    }

    ~Room()
    {
        DBG_LOG("%d room destory success.");
    }

    void handle_chess() // 处理下棋操作
    {

    }

    void handle_chat()
    {

    }

    void handle_exit()
    {

    }

    // 由大厅进入房间，用户已经准备就绪，所以不会有处理用户进入的函数

    void handle_request(Json::Value req)
    {

    }
};