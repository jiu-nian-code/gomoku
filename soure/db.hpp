#pragma once

#include"util.hpp"

#include<mutex>

class User_Table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    User_Table(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag)
    {
        bool ret = _mu.create_mysql(host, user, passwd, db, port, unix_socket, clientflag);
        if(!ret) DBG_LOG("mysql create fail.");
    }

    bool insert(Json::Value& va) // 注册
    {
        #define USER_TABLE_INSERT_USER_SQL "insert into user(name, password, score, total_games, win_games) value(\"%s\", MD5(\"%s\"), 1000, 0, 0);"
        if(va["name"].isNull() || va["password"].isNull())
        {
            DBG_LOG("name or password can't be null.");
            return false;
        }
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_INSERT_USER_SQL, va["name"].asCString(), va["password"].asCString());
        {
            std::unique_lock<std::mutex> lock(_mt);
            std::cout << buf << std::endl;
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("insert error.");
                return false;
            }
        }
        return true;
    }

    bool login(Json::Value& va)
    {
        #define USER_TABLE_SELECT_BY_NAME_AND_PW_SQL "select id, score, total_games, win_games from user where name = \"%s\" and password = MD5(\"%s\");"
        if(va["name"].isNull() || va["password"].isNull())
        {
            DBG_LOG("name or password can't be null.");
            return false;
        }
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_SELECT_BY_NAME_AND_PW_SQL, va["name"].asCString(), va["password"].asCString());
        MYSQL_RES* res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("select by name and pw error.");
                return false;
            }
            res = _mu.result_mysql();
            if(!res)
            {
                DBG_LOG("user information not exists!");
                return false;
            }
        }
        if(mysql_num_rows(res) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["id"] = std::stoi(rows[0]);
        va["score"] = std::stoi(rows[1]);
        va["total_games"] = std::stoi(rows[2]);
        va["win_games"] = std::stoi(rows[3]);
        mysql_free_result(res);
        return true;
    }

    bool select_by_name(const char* name, Json::Value& va)
    {
        #define USER_TABLE_SELECT_BY_NAME_SQL "select id, password, score, total_games, win_games from user where name = \"%s\";"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_SELECT_BY_NAME_SQL, name);
        MYSQL_RES* res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("select by name error.");
                return false;
            }
            res = _mu.result_mysql();
            if(!res)
            {
                DBG_LOG("result is empty.");
                return false;
            }
        }
        if(mysql_num_rows(res) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["id"] = std::stoi(rows[0]);
        va["name"] = std::string(name);
        va["score"] = std::stoi(rows[2]);
        va["total_games"] = std::stoi(rows[3]);
        va["win_games"] = std::stoi(rows[4]);
        mysql_free_result(res);
        return true;
    }

    bool select_by_uid(int uid, Json::Value& va)
    {
        #define USER_TABLE_SELECT_BY_ID_SQL "select name, password, score, total_games, win_games from user where id = %d;"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_SELECT_BY_ID_SQL, uid);
        MYSQL_RES* res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("select by id error.");
                return false;
            }
            res = _mu.result_mysql();
            if(!res)
            {
                DBG_LOG("result is empty.");
                return false;
            }
        }
        if(mysql_num_rows(res) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["name"] = rows[0];
        va["id"] = uid;
        va["score"] = std::stoi(rows[2]);
        va["total_games"] = std::stoi(rows[3]);
        va["win_games"] = std::stoi(rows[4]);
        mysql_free_result(res);
        return true;
    }

    bool win(int uid)
    {
        #define USER_TABLE_WIN_UPDATE "update user set score = score + 10, total_games = total_games + 1, win_games = win_games + 1 where id = %d;"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_WIN_UPDATE, uid);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("win update error.");
                return false;
            }
        }
        return true;
    }

    bool lose(int uid)
    {
        #define USER_TABLE_LOSE_UPDATE "update user set score = score - 10, total_games = total_games + 1 where id = %d;"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_LOSE_UPDATE, uid);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("win update error.");
                return false;
            }
        }
        return true;
    }
};

class matches_table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    matches_table(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag)
    {
        bool ret = _mu.create_mysql(host, user, passwd, db, port, unix_socket, clientflag);
        if(!ret) DBG_LOG("mysql create fail.");
    }

    bool select_by_uid(int uid, Json::Value& va)
    {
        #define MATCHES_TABLE_SELECT_BY_UID_SQL "select match_id, cur_score, is_win, cur_get, cur_date from user where id = \"%d\";"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_TABLE_SELECT_BY_UID_SQL, uid);
        MYSQL_RES* res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("select by id error.");
                return false;
            }
            res = _mu.result_mysql();
            if(!res)
            {
                DBG_LOG("result is empty.");
                return false;
            }
        }
        if(mysql_num_rows(res) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["match_id"] = std::stoi(rows[0]);
        va["id"] = uid;
        va["cur_score"] = std::stoi(rows[1]);
        va["is_win"] = std::stoi(rows[2]);
        va["cur_get"] = std::stoi(rows[3]);
        va["cur_date"] = rows[4];
        mysql_free_result(res);
        return true;
    }

    bool settlement(int uid, int cur_score, bool is_win, int cur_get)
    {
        #define USER_TABLE_WIN_UPDATE "insert into matches(uid, cur_score, is_win, cur_get) values(%d, %d, %d, %d)"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_WIN_UPDATE, uid, cur_score, is_win, cur_get);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("win update error.");
                return false;
            }
        }
        return true;
    }
};