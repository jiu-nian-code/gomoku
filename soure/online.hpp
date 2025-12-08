#include"websocketpp/server.hpp"
#include"websocketpp/config/asio_no_tls.hpp"
#include<unordered_map>
#include<mutex>
#include"util.hpp"

typedef websocketpp::server<websocketpp::config::asio> websocketsvr;

class online_manager
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