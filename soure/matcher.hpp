#pragma once
#include<iostream>
#include<list>
#include<mutex>
#include<condition_variable>
#include"session.hpp"
#include"db.hpp"
#include"room.hpp"

template<class T>
class Matcher_Queue
{
    std::list<T> _qu;
    std::mutex _mt;
    std::condition_variable _cv;
public:
    void push(T data)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _qu.push_back(data);
        _cv.notify_all();
    }

    bool pop(T& ret)
    {
        std::unique_lock<std::mutex> lock(_mt);
        if(_qu.empty()) return false;
        ret = _qu.front();
        _qu.pop_front();
        return true;
    }

    size_t size()
    {
        return _qu.size();
    }

    void remove(T data)
    {
        std::unique_lock<std::mutex> lock(_mt);
        _qu.remove(data);
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(_mt);
        _cv.wait(lock);
    }
};

class Matcher_Queue_Manager
{
    Matcher_Queue<int> _Bronze;
    Matcher_Queue<int> _Silver;
    Matcher_Queue<int> _Gold;
    std::thread _Bronze_th;;
    std::thread _Silver_th;
    std::thread _Gold_th;
    User_Table* _ut;
    Online_Manager* _om;
    Room_Manager* _rm;

    void handle_match(Matcher_Queue<int>& mq)
    {
        while(1)
        {
            while(mq.size() < 2) mq.wait();

            int uid1, uid2;
            if(!mq.pop(uid1))
            {
                ERR_LOG("uid1 pop fail.");
                continue;
            }
            if(!mq.pop(uid2))
            {
                ERR_LOG("uid2 pop fail.");
                mq.push(uid1); // uid2 pop失败，但是uid1成功了，这时失败返回要还原现场
                continue;
            }
            bool ret1 = _om->is_in_hall(uid1); // 判断是否掉线
            bool ret2 = _om->is_in_hall(uid2);
            if(!ret1 && !ret2)
            {
                ERR_LOG("uid1 uid2 disconnection.");
                continue;
            }
            if(!ret1)
            {
                ERR_LOG("uid1 disconnection.");
                mq.push(uid2);
                continue;
            }
            if(!ret2)
            {
                ERR_LOG("uid2 disconnection.");
                mq.push(uid1);
                continue;
            }
            // TODO
            if(uid1 == uid2)
            {
                ERR_LOG("the same user repeatedly joins the matching queue.");
                mq.push(uid1);
                continue;
            }
            Room_Manager::room_ptr rp = _rm->create_room(uid1, uid2);
            if(rp.get() == nullptr)
            {
                mq.push(uid1);
                mq.push(uid2);
            }
            Json::Value resp;
            resp["optype"] = "match_success";
            resp["result"] = true;
            std::string msg;
            Json_Util::serialize(resp, msg);
            websocketsvr::connection_ptr con1;
            websocketsvr::connection_ptr con2;
            _om->get_conn_from_hall(uid1, con1);
            _om->get_conn_from_hall(uid2, con2);
            con1->send(msg);
            con2->send(msg);
        }
    }

    void handle_Bronze() { handle_match(_Bronze); }
    void handle_Silver() { handle_match(_Silver); }
    void handle_Gold() { handle_match(_Gold); }
public:
    // User_Table* _ut;
    // Online_Manager* _om;
    // Room_Manager* _rm;
    Matcher_Queue_Manager(User_Table* ut, Online_Manager* om, Room_Manager* rm) :
        _Bronze_th(std::thread(&Matcher_Queue_Manager::handle_Bronze, this)),
        _Silver_th(std::thread(&Matcher_Queue_Manager::handle_Silver, this)),
        _Gold_th(std::thread(&Matcher_Queue_Manager::handle_Gold, this)),
        _ut(ut),
        _om(om),
        _rm(rm)
    {}

    bool add(int uid)
    {
        Json::Value va;
        if(!_ut->select_by_uid(uid, va))
        {
            ERR_LOG("select_by_id error");
            return false;
        }
        int score = va["score"].asInt();
        if(score < 2000)
        {
            _Bronze.push(uid);
        }
        else if(score >= 2000 && score < 3000)
        {
            _Silver.push(uid);
        }
        else
        {
            _Gold.push(uid);
        }
        return true;
    }

    bool del(int uid)
    {
        Json::Value va;
        if(!_ut->select_by_uid(uid, va))
        {
            ERR_LOG("select_by_id error");
            return false;
        }
        int score = va["score"].asInt();
        if(score < 2000)
        {
            _Bronze.remove(uid);
        }
        else if(score >= 2000 && score < 3000)
        {
            _Silver.remove(uid);
        }
        else
        {
            _Gold.remove(uid);
        }
        return true;
    }
};