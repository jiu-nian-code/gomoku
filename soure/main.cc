#include"util.hpp"

#include"db.hpp"

#include"online.hpp"

void test_Mysql_Util()
{
    Mysql_Util mu;
    mu.create_mysql("127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0);
    mu.exec_mysql("delete from stu where name = \"西巴砍\"");
    mu.exec_mysql("select * from stu");
    MYSQL_RES * res = mu.result_mysql();
    if(res)
    {
        // 打印表头
        my_ulonglong rownum = mysql_num_rows(res);
        unsigned int fieldnum = mysql_num_fields(res);
        MYSQL_FIELD * fields = mysql_fetch_fields(res);
        for(int i = 0; i < fieldnum; ++i)
            std::cout << fields[i].name << "\t";
        std::cout << std::endl;
        // 打印数据
        for(int i = 0; i < rownum; ++i)
        {
            MYSQL_ROW rows = mysql_fetch_row(res);
            for(int j = 0; j < fieldnum; ++j)
            {
                std::cout << rows[j] << "\t";
            }
            std::cout << std::endl;
        }
        mysql_free_result(res);
    }
}

void test_File_Util()
{
    std::string str;
    File_Util::read_file("./makefile", str);
    std::cout << str << std::endl;
    std::cout << str.size() << std::endl;
}

void test_user_table()
{
    User_Table ut("127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0);
    Json::Value va;
    Json::Value ret;
    va["name"] = "童宏旭";
    va["password"] = "thxTHX@0210";
    ut.insert(va);
    ut.login(va);
    // ut.win(va["id"].asInt());
    // ut.lose(va["id"].asInt());
    // std::cout << "id: " << va["id"].asInt() << " score: " 
    // << va["score"].asInt() << " total_games: " 
    // << va["total_games"] << " win_games: " 
    // << va["win_games"].asInt() << std::endl;
    // ut.select_by_name("田所浩二", va);
    // std::cout << "id: " << va["id"].asInt() << " score: " 
    // << va["score"].asInt() << " total_games: " 
    // << va["total_games"] << " win_games: " 
    // << va["win_games"].asInt() << std::endl;
    ut.select_by_id(va["id"].asInt(), ret);
    std::cout << "id: " << ret["id"].asInt() << " score: " 
    << ret["score"].asInt() << " total_games: " 
    << ret["total_games"] << " win_games: " 
    << ret["win_games"].asInt() << std::endl;
}

void test_online_manager()
{
    websocketsvr::connection_ptr con;
    online_manager om;
    std::cout << om.is_in_hall(1) << std::endl;
    om.enter_hall(1, con);
    std::cout << om.is_in_hall(1) << std::endl;
    om.exit_hall(1);
    std::cout << om.is_in_hall(1) << std::endl;
    std::cout << om.is_in_room(2) << std::endl;
    om.enter_room(2, con);
    std::cout << om.is_in_room(2) << std::endl;
    om.exit_room(2);
    std::cout << om.is_in_room(2) << std::endl;
}

int main()
{
    test_online_manager();
    return 0;
}