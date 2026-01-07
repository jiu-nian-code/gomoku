#pragma once
#include"online.hpp"

typedef enum { GAME_START, GAME_OVER } Game_Stu;
#define CHESSBOARD_SIZE 15
#define WHITE_COLOR 2
#define BLACK_COLOR 1
class Room
{
    int _room_id; // 房间id
    int _user_count; // 房间中的用户数量
    int _white_id; // 白棋方用户id
    int _black_id; // 黑棋方用户id
    int _cur_id;
    Online_Manager* _om; // 用户管理类
    User_Table* _ut;
    Matches_Table* _mtable;
    Matches_Step_Table* _mst;
    Game_Stu _gs;
    std::vector<std::vector<int>> _chessboard; // 棋盘，size: 15 * 15
    std::vector<std::pair<int, int>> _step;
    std::vector<std::string> _sensitive_word = 
    {
        "cnm",
        "操",
        "妈",
        "马",
        "日",
        "傻"
    };

    bool set_chess(int x, int y, int color)
    {
        if(_chessboard[x][y] != 0 || x >= CHESSBOARD_SIZE || y >= CHESSBOARD_SIZE) return false;
        _chessboard[x][y] = color;
        _step.emplace_back(x, y);
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
        while(_chessboard[a][b] == color) // 反向延伸
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
        five(x, y, color, 1, 1) || five(x, y, color, 1, -1)) return true;
        return false;
    }

    bool check_sensitive_word(const std::string& str)
    {
        for(auto& e : _sensitive_word)
        {
            size_t pos = str.find(e);
            if(pos != std::string::npos) return true;
        }
        return false;
    }
public:
    Room(int room_id, Online_Manager* om, User_Table* ut, Matches_Table* mtable) : 
        _room_id(room_id), _user_count(0),
        _white_id(-1), _black_id(-1), _cur_id(-1),
        _om(om), _ut(ut), _mtable(mtable), _gs(GAME_START),
        _chessboard(CHESSBOARD_SIZE, std::vector<int>(CHESSBOARD_SIZE, 0))
    {
        DBG_LOG("%d room create success.", _room_id);
    }

    ~Room()
    {
        DBG_LOG("%d room destory success.", _room_id);
    }

    // int _room_id; // 房间id
    // int _user_count; // 房间中的用户数量
    // int _white_id; // 白棋方用户id
    // int _black_id; // 黑棋方用户id
    // Online_Manager* _om; // 用户管理类
    // User_Table* _ut;
    // Game_Stu _gs;
    // std::vector<std::vector<int>> _chessboard; // 棋盘，size: 15 * 15
    void set_white(int uid) { _white_id = uid; ++_user_count; }
    void set_black(int uid) { _black_id = uid; ++_user_count; }
    void set_cur(int uid) { _cur_id = uid; }
    int get_cur() { return _cur_id; }
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
    Json::Value handle_chess(Json::Value& req) // 处理下棋操作
    {
        // 进来的前提: 1.room_id对上了 2.opt的if分支对上了
        Json::Value resp;
        resp["optype"] = req["optype"].asString();
        int uid = req["uid"].asInt();
        if(!_om->is_in_room(_white_id)) // 判断白方是否掉线 // TODO，有bug
        {
            DBG_LOG("white disconnected.");
            // resp["room_id"] = req["room_id"];
            // resp["uid"] = req["uid"];
            // resp["x"] = -1;
            // resp["y"] = -1;
            // resp["result"] = true;
            // resp["reason"] = "opposite side disconnected, you win";
            // resp["winner"] = _black_id;
            resp["uid"] = -2;
            return resp;
        }
        if(!_om->is_in_room(_black_id)) // 判断黑方是否掉线
        {
            DBG_LOG("black disconnected.");
            // resp["room_id"] = req["room_id"];
            // resp["uid"] = req["uid"];
            // resp["x"] = -1;
            // resp["y"] = -1;
            // resp["result"] = true;
            // resp["reason"] = "opposite side disconnected, you win"; // 掉线房间就剩一个人，所以都是己方胜利
            // resp["winner"] = _white_id;
            resp["uid"] = -1;
            return resp;
        }
        int x = req["x"].asInt();
        int y = req["y"].asInt();
        int color = uid == _white_id ? WHITE_COLOR : BLACK_COLOR;
        if(!set_chess(x, y, color))
        {
            DBG_LOG("position is not right, set chess fail.");
            resp["result"] = false;
            resp["reason"] = "position is not right, set chess fail.";
            return resp;
        }
        set_cur(uid == _white_id ? _black_id : _white_id);
        if(is_win(x, y, color))
        {
            resp["winner"] = uid;
            resp["result"] = true;
            resp["reason"] = "five in a row, victory!";
        }
        else
        {
            resp["winner"] = 0;
            resp["result"] = true;
        }
        resp["room_id"] = req["room_id"];
        resp["uid"] = req["uid"];
        resp["x"] = req["x"];
        resp["y"] = req["y"];
        return resp;
    }

    // chat json
    // {
    //     optype : ...
    //     roomid: ...
    //     uid: ...
    //     message : ...
    // }
    // chat success json
    // {
    //     optype : ...
    //     result : true
    //     roomid: ...
    //     uid: ...
    //     message : ...
    // }
    // chat fail json
    // {
    //     optype : ...
    //     result : false
    //     reason : ...
    // }
    Json::Value handle_chat(Json::Value& req)
    {
        check_req(req);
        Json::Value resp;
        std::string message = req["message"].asString();
        if(check_sensitive_word(message))
        {
            resp["optype"] = req["optype"].asString();
            resp["result"] = false;
            resp["reason"] = "containing sensitive words, not suitable for sending.";
            return resp;
        }
        resp = req;
        resp["result"] = true;
        return resp;
    }

    bool handle_exit(int uid)
    {
        std::cout << "handle_exit" << std::endl;
        if(_gs == GAME_START)
        {
            int winner = uid == _white_id ? _black_id : _white_id;
            Json::Value resp;
            resp["optype"] = "put_chess";
            resp["roomid"] = _room_id;
            resp["uid"] = uid;
            resp["winner"] = winner;
            resp["x"] = -1;
            resp["y"] = -1;
            resp["result"] = true;
            resp["reason"] = "opposite side disconnected, you win";

            // int loser = winner == _white_id ? _black_id : _white_id;
            // _ut->win(winner);
            // _ut->lose(loser);
            int loser = winner == _white_id ? _black_id : _white_id;
            if(!_ut->win(winner)) { DBG_LOG("win error."); return false; }
            if(!_ut->lose(loser)) { DBG_LOG("lose error."); return false; }
            Json::Value va1;
            if(!_ut->select_by_uid(winner, va1)) { DBG_LOG("select_by_uid error."); return false; }
            Json::Value va2;
            if(!_ut->select_by_uid(loser, va2)) { DBG_LOG("select_by_uid error."); return false; }
            if(winner == _white_id)
            {
                if(!_mtable->settlement(_white_id, _black_id, va1["score"].asInt(), va2["score"].asInt(), 10, -10, true))
                { DBG_LOG("settlement error."); return false; }
            }
            else
            {
                if(!_mtable->settlement(_white_id, _black_id, va2["score"].asInt(), va1["score"].asInt(), -10, 10, false))
                { DBG_LOG("settlement error."); return false; }
            }
            // _mst->settlement(_mtable->get_mid_by_uid(winner), _step);
            _gs = GAME_OVER;
            broad_cast(resp);
        }
        --_user_count;
        return true;
    }

    // 由大厅进入房间，用户已经准备就绪，所以不会有处理用户进入的函数

    // int _room_id; // 房间id
    // int _user_count; // 房间中的用户数量
    // int _white_id; // 白棋方用户id
    // int _black_id; // 黑棋方用户id
    // Online_Manager* _om; // 用户管理类
    // User_Table* _ut;
    // Game_Stu _gs;
    // std::vector<std::vector<int>> _chessboard; // 棋盘，size: 19 * 19

    bool check_req(Json::Value& req)
    {
        Json::Value resp;
        int room_id = req["room_id"].asInt();
        if(room_id != _room_id)
        {
            DBG_LOG("room id mismatch.");
            resp["optype"] = req["optype"].asString();
            resp["result"] = false;
            resp["reason"] = "room id mismatch.";
            broad_cast(resp);
            return false;
        }
        int uid = req["uid"].asInt();
        if(_white_id != uid && _black_id != uid)
        {
            DBG_LOG("operator user is not exists.");
            resp["result"] = false;
            resp["reason"] = "operator user is not exists.";
            broad_cast(resp);
            return false;
        }
        return true;
    }

    int handle_request(Json::Value& req) // 请求处理分发
    {
        check_req(req);
        Json::Value resp;
        std::string opt = req["optype"].asString();
        if(opt == "put_chess")
        {
            resp = handle_chess(req);
            if(resp["uid"].asInt() == -1) return 1;
            if(resp["uid"].asInt() == -2) return 2;
            int winner = resp["winner"].asInt();
            if(winner != 0)
            {
                int loser = winner == _white_id ? _black_id : _white_id;
                if(!_ut->win(winner)) { DBG_LOG("win error."); return false; }
                if(!_ut->lose(loser)) { DBG_LOG("lose error."); return false; }
                Json::Value va1;
                if(!_ut->select_by_uid(winner, va1)) { DBG_LOG("select_by_uid error."); return false; }
                Json::Value va2;
                if(!_ut->select_by_uid(loser, va2)) { DBG_LOG("select_by_uid error."); return false; }
                if(winner == _white_id)
                {
                    if(!_mtable->settlement(_white_id, _black_id, va1["score"].asInt(), va2["score"].asInt(), 10, -10, true))
                    { DBG_LOG("settlement error."); return false; }
                }
                else
                {
                    if(!_mtable->settlement(_white_id, _black_id, va2["score"].asInt(), va1["score"].asInt(), -10, 10, false))
                    { DBG_LOG("settlement error."); return false; }
                }
                // _mst->settlement(_mtable->get_mid_by_uid(winner), _step);
                _gs = GAME_OVER;
            }
        }
        else if(opt == "chat")
        {
            resp = handle_chat(req);
        }
        else
        {
            resp["optype"] = req["optype"].asString();
            resp["result"] = false;
            resp["reason"] = "unknow optype";
        }
        broad_cast(resp);
        return 0;
    }

    void broad_cast(Json::Value& resp) // 将用户的操作广播给同一个房间的其他用户
    {
        std::string str;
        if(!Json_Util::serialize(resp, str)) { DBG_LOG("broadcast fail."); return; }
        websocketsvr::connection_ptr white_con;
        websocketsvr::connection_ptr black_con;
        if(_om->get_conn_from_room(_white_id, white_con)) white_con->send(str);
        else DBG_LOG("get white connection fail.");
        if(_om->get_conn_from_room(_black_id, black_con)) black_con->send(str);
        else DBG_LOG("get black connection fail.");
    }

    void board_reset(Json::Value& resp) // 掉线重连恢复信息
    {
        for(auto& m : _chessboard)
            for(auto& n : m)
                resp["chessboard"].append(n);
        resp["cur_uid"] = get_cur();
    }
};

class Room_Manager
{
public:
    using room_ptr = std::shared_ptr<Room>;
private:
    Online_Manager* _om;
    User_Table* _ut;
    Matches_Table* _mtable;
    int _room_id_alloc;
    // using room_ptr = std::shared_ptr<Room>;
    std::unordered_map<int, room_ptr> _rid_room;
    std::unordered_map<int, int> _user_room;
    std::mutex _mt;
public:
    Room_Manager(Online_Manager* om, User_Table* ut, Matches_Table* mtable) : _om(om), _ut(ut), _mtable(mtable), _room_id_alloc(0) {}
    ~Room_Manager() {}

    room_ptr create_room(int user1, int user2) // 匹配成功使用两位用户id创建房间
    {
        std::cout << "create_room" << std::endl;
        if(!_om->is_in_hall(user1))
        {
            DBG_LOG("user %d is not in hall.", user1);
            return room_ptr();
        }
        if(!_om->is_in_hall(user2))
        {
            DBG_LOG("user %d is not in hall.", user2);
            return room_ptr();
        }
        room_ptr tmp(new Room(_room_id_alloc, _om, _ut, _mtable));
        std::unique_lock<std::mutex> lock(_mt);
        tmp->set_white(user1);
        tmp->set_black(user2);
        _rid_room[_room_id_alloc] = tmp;
        _user_room[user1] = _room_id_alloc;
        _user_room[user2] = _room_id_alloc;
        ++_room_id_alloc;
        return tmp;
    }

    room_ptr get_room_by_rid(int rid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it = _rid_room.find(rid);
        if(it == _rid_room.end())
        {
            DBG_LOG("room%d is not exists.", rid);
            return room_ptr();
        }
        return it->second;
    }

    // int get_rid_by_uid(int uid)
    // {
    //     std::unique_lock<std::mutex> lock(_mt);
    //     auto it1 = _user_room.find(uid);
    //     if(it1 == _user_room.end())
    //     {
    //         DBG_LOG("user %d is not in room.", uid);
    //         return -1;
    //     }
    //     return it1->second;
    // }

    room_ptr get_room_by_uid(int uid)
    {
        std::unique_lock<std::mutex> lock(_mt);
        auto it1 = _user_room.find(uid);
        if(it1 == _user_room.end())
        {
            DBG_LOG("user %d is not in room.", uid);
            return room_ptr();
        }
        auto it2 = _rid_room.find(it1->second);
        if(it2 == _rid_room.end())
        {
            DBG_LOG("room %d is not in room.", it1->second);
            return room_ptr();
        }
        return it2->second;
    }

    bool remove_user_room(int uid) // 用户退出房间
    {
        std::cout << "remove user room" << std::endl;
        room_ptr rp = get_room_by_uid(uid);
        if(rp.get() == nullptr) return false;
        if(!_om->is_in_room(uid)) rp->handle_exit(uid); // 防止按钮退出已经不在房间中了
        if(rp->get_user_count() == 0) remove_room(rp->get_room_id());
        return true;
    }

    bool remove_room(int rid)
    {
        // std::cout << "remove room" << std::endl;
        room_ptr rp = get_room_by_rid(rid);
        if(rp.get() == nullptr) return false;
        int white = rp->get_white();
        int black = rp->get_black();
        std::unique_lock<std::mutex> lock(_mt);
        _user_room.erase(white);
        _user_room.erase(black);
        _rid_room.erase(rid);
        return true;
    }
};