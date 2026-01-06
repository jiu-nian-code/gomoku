// #include"util.hpp"
// #include"db.hpp"
// #include"online.hpp"
// #include"room.hpp"
// #include"session.hpp"
// #include"matcher.hpp"
// #include"matcher.hpp"
#include"server.hpp"

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
    ut.select_by_uid(va["id"].asInt(), ret);
    std::cout << "id: " << ret["id"].asInt() << " score: " 
    << ret["score"].asInt() << " total_games: " 
    << ret["total_games"] << " win_games: " 
    << ret["win_games"].asInt() << std::endl;
}

void test_online_manager()
{
    websocketsvr::connection_ptr con;
    Online_Manager om;
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

void test_room()
{
    Online_Manager om;
    User_Table ut("127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0);
    websocketsvr::connection_ptr cp1;
    websocketsvr::connection_ptr cp2;
    om.enter_hall(1, cp1);
    om.enter_hall(2, cp2);
    Room_Manager rm(&om, &ut);
    rm.create_room(1, 2);
    rm.remove_user_room(1);
    rm.remove_user_room(2);
    Room_Manager::room_ptr rp = rm.get_room_by_uid(1);
    if(rp.get() == nullptr) std::cout << "haha" << std::endl;
}

Session_Manager* sm_ptr = nullptr;

void handler_http(websocketsvr* server, websocketpp::connection_hdl hdl)
{
    // std::cout << sm_ptr->is_in_session(0) << std::endl;
    session_ptr sp = sm_ptr->create_session(1, LOGIN);
    sm_ptr->set_session_expire_time(sp->get_sid(), -1);
    sm_ptr->set_session_expire_time(sp->get_sid(), 3000);
    sleep(4);
    // std::cout << sm_ptr->is_in_session(sp->get_sid()) << std::endl;
    // sleep(10);
    // std::cout << 3 << std::endl;
}

void handler_message(websocketsvr* server, websocketpp::connection_hdl hdl, std::shared_ptr<websocketpp::config::core::message_type> msg) {}

void handler_open(websocketpp::connection_hdl hdl) {}

void handler_close(websocketpp::connection_hdl hdl) {}

void test_session()
{
    websocketsvr server;
    server.set_access_channels(websocketpp::log::alevel::none);
    server.init_asio();
    server.set_reuse_addr(true);
    server.set_http_handler(std::bind(&handler_http, &server, std::placeholders::_1));
    server.set_open_handler(handler_open);
    server.set_close_handler(handler_close);
    server.set_message_handler(std::bind(&handler_message, &server, std::placeholders::_1, std::placeholders::_2));
    server.listen(9091);
    server.start_accept();
    Session_Manager sm(&server);
    sm_ptr = &sm;
        // --- 开始设置多线程 ---
    int thread_count = 4; // 比如设置 4 个线程
    std::vector<std::thread> threads;

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&server]() {
            // 多个线程同时竞争处理 io_service 里的任务
            server.run(); 
        });
    }

    // 主线程可以去处理别的事情，或者等待这些线程结束
    for (auto& t : threads) {
        t.join();
    }
}

void test_matcher()
{
    Matcher_Queue<int> mq;
    mq.push(66);
    int x = 0;
    mq.pop(x);
    std::cout << x << std::endl;
    // mq.remove();
}

void test_server()
{
    Gomoku_Server gs("127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0, "WWWROOT");
    gs.start();
}

int main()
{
    test_server();
    return 0;
}