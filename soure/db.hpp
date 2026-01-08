#pragma once

#include"util.hpp"

#include<mutex>

class User_Avatar_Table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    User_Avatar_Table(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag)
    {
        bool ret = _mu.create_mysql(host, user, passwd, db, port, unix_socket, clientflag);
        if(!ret) DBG_LOG("mysql create fail.");
    }

    int get_aid_by_uid(int uid)
    {
        #define AVATAR_TABLE_GET_AID_BY_UID "select avatar_id from user_avatar where id = %d"
        char buf[1024] = {0};
        snprintf(buf, 1024, AVATAR_TABLE_GET_AID_BY_UID, uid);
        MYSQL_RES* res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("select by id error.");
                return -1;
            }
            res = _mu.result_mysql();
            if(!res)
            {
                DBG_LOG("result is empty.");
                return -1;
            }
        }
        if(mysql_num_rows(res) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return -1;
        }
        MYSQL_ROW rows = mysql_fetch_row(res);
        int aid = std::stoi(rows[0]);
        mysql_free_result(res);
        return aid;
    }

    bool insert_avatar_by_uid(int uid)
    {
        #define AVATAR_TABLE_INSERT_AVATAR_BY_UID "insert into user_avatar(id) value(%d)"
        char buf[1024] = {0};
        snprintf(buf, 1024, AVATAR_TABLE_INSERT_AVATAR_BY_UID, uid);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("insert_avatar_by_uid error.");
                return false;
            }
        }
        return true;
    }
};

class Matches_Table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    Matches_Table(const char *host, const char *user, 
        const char *passwd, const char *db, 
        unsigned int port, const char *unix_socket, 
        unsigned long clientflag)
    {
        bool ret = _mu.create_mysql(host, user, passwd, db, port, unix_socket, clientflag);
        if(!ret) DBG_LOG("mysql create fail.");
    }

    bool select_by_uid(int uid, Json::Value& va)
    {
        #define MATCHES_TABLE_SELECT_BY_UID_SQL \
        "select match_id, white_uid, white_cur_score, "\
        "black_cur_score, white_cur_get, black_cur_get, is_white_win, "\
        "cur_date from matches where white_uid = %d or black_uid = %d order by match_id desc limit 20"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_TABLE_SELECT_BY_UID_SQL, uid, uid);
        std::cout << buf << std::endl;
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
        for(int i = 0; i < mysql_num_rows(res); ++i)
        {
            MYSQL_ROW rows = mysql_fetch_row(res);
            Json::Value tmp;
            tmp["match_id"] = std::stoull(rows[0]);
            tmp["id"] = uid;
            if(uid == std::stoi(rows[1])) // 是白棋id
            {
                tmp["cur_score"] = std::stoi(rows[2]);
                tmp["is_win"] = std::stoi(rows[6]);
                tmp["cur_get"] = std::stoi(rows[4]);
            }
            else
            {
                tmp["cur_score"] = std::stoi(rows[3]);
                tmp["is_win"] = !(std::stoi(rows[6]));
                tmp["cur_get"] = std::stoi(rows[5]);
            }
            tmp["cur_date"] = rows[7];
            std::string str;
            Json_Util::serialize(tmp, str);
            std::cout << str << std::endl;
            va["matches"].append(str);
        }
        mysql_free_result(res);
        return true;
    }

    bool select_by_mid(int uid, Json::Value& va)
    {
        #define MATCHES_TABLE_SELECT_BY_MID_SQL \
        "select white_uid, black_uid, white_cur_score, "\
        "black_cur_score, white_cur_get, black_cur_get, is_white_win, "\
        "cur_date from matches where match_id = %d"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_TABLE_SELECT_BY_MID_SQL, uid, uid);
        std::cout << buf << std::endl;
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
        va["white_uid"] = std::stoi(rows[0]);
        va["black_uid"] = std::stoi(rows[1]);
        va["white_cur_score"] = std::stoi(rows[2]);
        va["black_cur_score"] = std::stoi(rows[3]);
        va["white_cur_get"] = std::stoi(rows[4]);
        va["black_cur_get"] = std::stoi(rows[5]);
        va["is_white_win"] = std::stoi(rows[6]);
        va["cur_date"] = rows[7];
        mysql_free_result(res);
        return true;
    }

    // create table if not exists matches
    // (
    //     match_id bigint unsigned primary key auto_increment comment "对局ID",
    //     white_uid int comment "白棋用户id",
    //     black_uid int comment "黑棋用户id",
    //     white_cur_score int comment "白棋用户当前总分",
    //     black_cur_score int comment "黑棋用户当前总分",
    //     white_cur_get int comment "白棋用户当前得分",
    //     black_cur_get int comment "黑棋用户当前得分",
    //     is_white_win bool comment "是否是白棋赢",
    //     cur_date timestamp
    // );

    bool settlement(int white_uid, int black_uid, int white_cur_score, int black_cur_score, int white_cur_get, int black_cur_get, bool is_white_win)
    {
        #define MATCHES_SETTLEMENT_TABLE_UPDATE_SQL \
        "insert into matches(white_uid, black_uid, white_cur_score, black_cur_score, white_cur_get, black_cur_get, is_white_win) values(%d, %d, %d, %d, %d, %d, %d)"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_SETTLEMENT_TABLE_UPDATE_SQL, white_uid, black_uid, white_cur_score, black_cur_score, white_cur_get, black_cur_get, is_white_win);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("settlement error.");
                return false;
            }
        }
        return true;
    }

    unsigned long long get_mid_by_uid(int uid)
    {
        #define MATCHES_GET_MID_BY_UID_TABLE_UPDATE_SQL "select match_id from matches where white_uid = %d or black_uid = %d order by match_id desc limit 1"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_GET_MID_BY_UID_TABLE_UPDATE_SQL, uid, uid);
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
        unsigned long long mid = std::stoull(rows[0]);
        return mid;
    }
};

class Matches_Step_Table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    Matches_Step_Table(const char *host, const char *user, 
    const char *passwd, const char *db, 
    unsigned int port, const char *unix_socket, 
    unsigned long clientflag)
    {
        bool ret = _mu.create_mysql(host, user, passwd, db, port, unix_socket, clientflag);
        if(!ret) DBG_LOG("mysql create fail.");
    }

    bool select_by_mid(int mid, Json::Value& va)
    {
        #define MATCHES_STEP_TABLE_SELECT_BY_UID_SQL "select x, y from matches_step where match_id = %d order by step asc"
        char buf[1024] = {0};
        snprintf(buf, 1024, MATCHES_STEP_TABLE_SELECT_BY_UID_SQL, mid);
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
        for(int i = 0; i < mysql_num_rows(res); ++i)
        {
            MYSQL_ROW rows = mysql_fetch_row(res);
            Json::Value tmp;
            tmp["x"] = rows[0];
            tmp["y"] = rows[1];
            std::string str;
            Json_Util::serialize(tmp, str);
            std::cout << str << std::endl;
            va["step"].append(str);
        }
        mysql_free_result(res);
        return true;
    }

    bool settlement(unsigned long long mid, const std::vector<std::pair<int, int>>& steps)
    {
        #define MATCHES_STEP_SETTLEMENT_TABLE_UPDATE_SQL "insert into matches_step values(%llu, %d, %d, %d)"
        char buf[1024] = {0};
        // snprintf(buf, 1024, MATCHES_SETTLEMENT_TABLE_UPDATE_SQL, mid, oppo_uid, cur_score, is_win, is_white, cur_get);
        {
            std::unique_lock<std::mutex> lock(_mt);
            size_t num = 0;
            for(auto& e : steps)
            {
                snprintf(buf, 1024, MATCHES_STEP_SETTLEMENT_TABLE_UPDATE_SQL, mid, e.first, e.second, num++);
                bool ret = _mu.exec_mysql(buf);
                if(!ret)
                {
                    DBG_LOG("win update error.");
                    return false;
                }
            }
        }
        return true;
    }
};

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
        #define USER_TABLE_INSERT_USER_SQL "insert into user(name, password, score, total_games, win_games) value(\"%s\", MD5(\"%s\"), 1000, 0, 0)"
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
        #define USER_TABLE_SELECT_BY_NAME_AND_PW_SQL "select id, score, total_games, win_games from user where name = \"%s\" and password = MD5(\"%s\")"
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
        #define USER_TABLE_SELECT_BY_NAME_SQL "select id, password, score, total_games, win_games from user where name = \"%s\""
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
        #define USER_TABLE_SELECT_BY_ID_SQL "select name, password, score, total_games, win_games from user where id = %d"
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

    bool select_ntw_by_uid(int w_uid, int b_uid, Json::Value& va)
    {
        #define USER_TABLE_SELECT_NTW_BY_ID_SQL "select name, total_games, win_games from user where id = %d"
        char wbuf[1024] = {0};
        snprintf(wbuf, 1024, USER_TABLE_SELECT_NTW_BY_ID_SQL, w_uid);
        char bbuf[1024] = {0};
        snprintf(bbuf, 1024, USER_TABLE_SELECT_NTW_BY_ID_SQL, b_uid);
        MYSQL_RES* wres = nullptr;
        MYSQL_RES* bres = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool wret = _mu.exec_mysql(wbuf);
            if(!wret)
            {
                DBG_LOG("select by id error.");
                return false;
            }
            wres = _mu.result_mysql();
            if(!wres)
            {
                DBG_LOG("result is empty.");
                return false;
            }
            bool bret = _mu.exec_mysql(bbuf);
            if(!bret)
            {
                DBG_LOG("select by id error.");
                return false;
            }
            bres = _mu.result_mysql();
            if(!bres)
            {
                DBG_LOG("result is empty.");
                return false;
            }
        }
        if(mysql_num_rows(wres) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        if(mysql_num_rows(bres) != 1)
        {
            DBG_LOG("user information is duplicated!");
            return false;
        }
        MYSQL_ROW wrows = mysql_fetch_row(wres);
        MYSQL_ROW brows = mysql_fetch_row(bres);
        va["white_name"] = wrows[0];
        va["white_total_games"] = std::stoi(wrows[1]);
        va["white_win_games"] = std::stoi(wrows[2]);
        va["black_name"] = brows[0];
        va["black_total_games"] = std::stoi(brows[1]);
        va["black_win_games"] = std::stoi(brows[2]);
        mysql_free_result(wres);
        mysql_free_result(bres);
        return true;
    }

    bool win(int uid)
    {
        #define USER_TABLE_WIN_UPDATE_SQL "update user set score = score + 10, total_games = total_games + 1, win_games = win_games + 1 where id = %d"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_WIN_UPDATE_SQL, uid);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("win update error.");
                return false;
            }
        }
        // Json::Value va;
        // if(!select_by_uid(uid, va))
        // {
        //     DBG_LOG("select_by_uid error.");
        //     return false;
        // }
        // std::cout << va["score"].asInt() << std::endl;
        // std::cout << _mtable << std::endl;
        // _mtable->settlement(uid, va["score"].asInt(), true, 10);
        return true;
    }

    bool lose(int uid)
    {
        #define USER_TABLE_LOSE_UPDATE_SQL "update user set score = score - 10, total_games = total_games + 1 where id = %d"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_LOSE_UPDATE_SQL, uid);
        {
            std::unique_lock<std::mutex> lock(_mt);
            bool ret = _mu.exec_mysql(buf);
            if(!ret)
            {
                DBG_LOG("win update error.");
                return false;
            }
        }
        // Json::Value va;
        // if(!select_by_uid(uid, va))
        // {
        //     DBG_LOG("select_by_uid error.");
        //     return false;
        // }
        // std::cout << va["score"].asInt() << std::endl;
        // std::cout << _mtable << std::endl;
        // _mtable->settlement(uid, va["score"].asInt(), false, -10);
        return true;
    }

    bool get_top_k(int k, Json::Value& va)
    {
        #define USER_TABLE_GET_TOP_K_SQL "select name, score, id from user order by score desc limit %d"
        char buf[1024] = {0};
        snprintf(buf, 1024, USER_TABLE_GET_TOP_K_SQL, k);
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
        // if(mysql_num_rows(res) != 1)
        // {
        //     DBG_LOG("user information is duplicated!");
        //     return false;
        // }
        for(int i = 0; i < mysql_num_rows(res); ++i)
        {
            MYSQL_ROW rows = mysql_fetch_row(res);
            Json::Value tmp;
            tmp["name"] = rows[0];
            tmp["score"] = std::stoi(rows[1]);
            tmp["id"] = std::stoi(rows[2]);
            std::string str;
            Json_Util::serialize(tmp, str);
            va["users"].append(str);
        }
        mysql_free_result(res);
        return true;
    }
};