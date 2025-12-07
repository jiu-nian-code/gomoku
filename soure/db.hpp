#include"util.hpp"

#include<mutex>

class User_Table
{
    Mysql_Util _mu;
    std::mutex _mt;
public:
    User_Table()
    {
        _mu.create_mysql("127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0);
    }

    bool insert(const char* name, const char* password) // 注册
    {
        #define INSERT_USER_SQL "insert into user(name, password, score, total_games, win_games) value(%s, password(\"%s\"), 0, 0, 0);"
        if(!name || !password)
        {
            DBG_LOG("name or password can't be null.");
            return false;
        }
        char buf[1024] = {0};
        snprintf(buf, 1024, INSERT_USER_SQL, name, password);
        bool ret = _mu.exec_mysql(buf);
        if(!ret)
        {
            DBG_LOG("insert error.");
            return false;
        }
        return true;
    }

    bool login(const char* name, const char* password)
    {
        #define SELECT_BY_NAME_AND_PW_SQL "select id, score, total_games, win_games from user where name = \"%s\" and password = pqssword(\"%s\");"
        if(!name || !password)
        {
            DBG_LOG("name or password can't be null.");
            return false;
        }
        char buf[1024] = {0};
        snprintf(buf, 1024, SELECT_BY_NAME_AND_PW_SQL, name, password);
        bool ret = _mu.exec_mysql(buf);
        if(!ret)
        {
            DBG_LOG("select by name and pw error.");
            return false;
        }
        MYSQL_RES* res = _mu.result_mysql();
        if(!res || mysql_num_fields(res) != 1)
        {
            DBG_LOG("");
        }
        return true;
    }

    bool select_by_name(const char* name, Json::Value& va)
    {
        #define SELECT_BY_NAME_SQL "select id, password, score, total_games, win_games from user where name = \"%s\";"
        char buf[1024] = {0};
        snprintf(buf, 1024, INSERT_USER_SQL, name);
        bool ret = _mu.exec_mysql(buf);
        if(!ret)
        {
            DBG_LOG("select by name error.");
            return false;
        }
        MYSQL_RES* res = _mu.result_mysql();
        if(!res)
        {
            DBG_LOG("result is empty.");
            return false;
        }
        my_ulonglong rownum = mysql_num_rows(res);
        unsigned int fieldnum = mysql_num_fields(res);
        if(fieldnum != 1) return false;
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["id"] = rows[0];
        va["password"] = rows[1];
        va["score"] = std::stoi(rows[2]);
        va["total_games"] = std::stoi(rows[3]);
        va["win_games"] = std::stoi(rows[4]);
        mysql_free_result(res);
        return true;
    }

    bool select_by_id(int id, Json::Value& va)
    {
        #define SELECT_BY_NAME_SQL "select id, password, score, total_games, win_games from user where id = %d;"
        char buf[1024] = {0};
        snprintf(buf, 1024, INSERT_USER_SQL, id);
        bool ret = _mu.exec_mysql(buf);
        if(!ret)
        {
            DBG_LOG("select by id error.");
            return false;
        }
        MYSQL_RES* res = _mu.result_mysql();
        if(!res)
        {
            DBG_LOG("result is empty.");
            return false;
        }
        my_ulonglong rownum = mysql_num_rows(res);
        unsigned int fieldnum = mysql_num_fields(res);
        if(fieldnum != 1) return false;
        MYSQL_ROW rows = mysql_fetch_row(res);
        va["name"] = rows[0];
        va["password"] = rows[1];
        va["score"] = std::stoi(rows[2]);
        va["total_games"] = std::stoi(rows[3]);
        va["win_games"] = std::stoi(rows[4]);
        mysql_free_result(res);
        return true;
    }

    void win()
    {

    }

    void lose()
    {

    }
};