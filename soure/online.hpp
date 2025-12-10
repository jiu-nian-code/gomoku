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
#define CHESSBOARD_SIZE 15
#define WHITE_COLOR 1
#define BLACK_COLOR 2
class Room
{
    int _room_id; // 房间id
    int _user_count; // 房间中的用户数量
    int _white_id; // 白棋方用户id
    int _black_id; // 黑棋方用户id
    Online_Manager* _om; // 用户管理类
    User_Table* _ut;
    Game_Stu _gs;
    std::vector<std::vector<int>> _chessboard; // 棋盘，size: 15 * 15

    bool set_chess(int x, int y, int color)
    {
        if(_chessboard[x][y] != 0 || x >= CHESSBOARD_SIZE || y >= CHESSBOARD_SIZE) return false;
        _chessboard[x][y] = color;
        return true;
    }

    bool five(int x, int y, int color, int x_offset, int y_offset)
    {
        int sum = 0;
        int a = x;
        int b = y;
        while(_chessboard[a][b] == color) // 正向延伸
        {
            ++sum;
            a += x_offset;
            b += y_offset;
            if(a < 0 || a >= CHESSBOARD_SIZE || b < 0 || b >= CHESSBOARD_SIZE) break;
        }
        a = x;
        b = y;
        while(_chessboard[a][b] == color) // 正向延伸
        {
            ++sum;
            a -= x_offset;
            b -= y_offset;
            if(a < 0 || a >= CHESSBOARD_SIZE || b < 0 || b >= CHESSBOARD_SIZE) break;
        }
        --sum; // 中心点多算了一次
        if(sum >= 5) return true;
        return false;
    }

    bool is_win(int x, int y, int color)
    {
        if(five(x, y, color, 0, 1) || five(x, y, color, 1, 0) || 
        five(x, y, color, 1, 1) || five(x, y, color, -1, -1)) return true;
        return false;
    }
public:
    Room(int room_id, Online_Manager* om, User_Table* ut) : 
        _room_id(room_id), _user_count(0),
        _white_id(-1), _black_id(-1),
        _om(om), _ut(ut), _gs(GAME_START),
        _chessboard(CHESSBOARD_SIZE, std::vector<int>(CHESSBOARD_SIZE, 0))
    {
        DBG_LOG("%d room create success.");
    }

    ~Room()
    {
        DBG_LOG("%d room destory success.");
    }

    // int _room_id; // 房间id
    // int _user_count; // 房间中的用户数量
    // int _white_id; // 白棋方用户id
    // int _black_id; // 黑棋方用户id
    // Online_Manager* _om; // 用户管理类
    // User_Table* _ut;
    // Game_Stu _gs;
    // std::vector<std::vector<int>> _chessboard; // 棋盘，size: 19 * 19
    void set_white(int id) { _white_id = id; ++_user_count; }
    void set_black(int id) { _black_id = id; ++_user_count; }
    int get_white() { return _white_id; }
    int get_black() { return _black_id; }
    int get_room_id() { return _room_id; }
    int get_user_count() { return _user_count; }
    Game_Stu get_game_status() { return _gs; }

    // set chess json
    // {
    //     optype : ...
    //     roomid: ...
    //     uid: ...
    //     x : ...
    //     y : ...
    // }
    // set chess success json
    // {
    //     optype : ...
    //     result : true
    //     reason : ... // 没赢没出错，只是普通下棋不需要原因
    //     roomid: ...
    //     uid: ...
    //     x : ...
    //     y : ...
    //     winner : ... // 为0没有人赢
    // }
    // set chess fail json
    // {
    //     optype : ...
    //     result : false
    //     reason : ...
    // }
    // white : 1
    // black : 2
    Json::Value& handle_chess(Json::Value& req) // 处理下棋操作
    {
        // 进来的前提: 1.room_id对上了 2.opt的if分支对上了
        Json::Value resp;
        resp["optype"] = req["optype"].asString();
        int uid = req["uid"].asInt();
        if(_white_id != uid || _black_id != uid) // 其实有点没必要...
        {
            DBG_LOG("operator user is not exists.");
            resp["result"] = false;
            resp["reason"] = "operator user is not exists.";
            return resp;
        }
        if(_om->is_in_room(_white_id)) // 判断操作方是否掉线
        {
            DBG_LOG("white disconnected.");
            resp["result"] = true;
            resp["reason"] = "对方掉线，己方胜利";
            resp["winner"] = _white_id;
            return resp;
        }
        if(_om->is_in_room(_black_id)) // 判断对方是否掉线
        {
            DBG_LOG("black disconnected.");
            resp["result"] = true;
            resp["reason"] = "对方掉线，己方胜利"; // 掉线房间就剩一个人，所以都是己方胜利
            resp["winner"] = _black_id;
            return resp;
        }
        int x = req["x"].asInt();
        int y = req["y"].asInt();
        int color = uid == _white_id ? WHITE_COLOR : BLACK_COLOR;
        if(set_chess(x, y, color))
        {
            DBG_LOG("position is not right, set chess fail.");
            resp["result"] = false;
            resp["reason"] = "position is not right, set chess fail.";
            return resp;
        }
        if(is_win(x, y, color))
        {
            resp["winner"] = uid;
            resp["result"] = true;
            resp["reason"] = "五子连珠，胜利！";
        }
        else
        {
            resp["winner"] = 0;
            resp["result"] = true;
        }
        return resp;
    }

    Json::Value& handle_chat(Json::Value& req)
    {

    }

    void handle_exit(int id)
    {

    }

    // 由大厅进入房间，用户已经准备就绪，所以不会有处理用户进入的函数

    void handle_request(Json::Value& req) // 请求处理分发
    {
        Json::Value resp;
        int room_id = req["room_id"].asInt();
        if(room_id != _room_id)
        {
            DBG_LOG("room id mismatch.");
            resp["optype"] = req["optype"].asString();
            resp["result"] = false;
            resp["reason"] = "room id mismatch.";
            return broad_cast(resp);
        }
        std::string opt = req["optype"].asString();
        if(opt == "put_chess")
        {
            
        }
        else if(opt == "chat")
        {

        }
        else if(opt == "exit")
        {

        }

        broad_cast(resp);
    }

    void broad_cast(Json::Value& resp) // 将用户的操作广播给同一个房间的其他用户
    {

    }
};