#include"util.hpp"

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

int main()
{
    test_File_Util();
    return 0;
}