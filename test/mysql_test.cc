#include<iostream>

#include<mysql/mysql.h>

int main()
{
    // MYSQL *mysql_init(MYSQL *mysql);
    MYSQL mq;
    MYSQL* ret = mysql_init(&mq);
    if(!ret) { perror("mysql_init"); exit(1); }
    ret = mysql_real_connect(&mq, "127.0.0.1", "thx", "thxTHX@0210", "gomoku_db", 3333, NULL, 0);
    if(!ret) { perror("mysql_real_connect"); exit(1); }
    // int mysql_set_character_set(MYSQL *mysql, const char *csname)
    int back = mysql_set_character_set(&mq, "utf8");
    if(back != 0)  { perror("mysql_set_character_set"); exit(1); }
    // int mysql_query(MYSQL *mysql, const char *stmt_str)
    mysql_query(&mq, "select * from matches");
    MYSQL_RES* res = mysql_store_result(&mq);
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
    mysql_close(&mq);
    return 0;
}