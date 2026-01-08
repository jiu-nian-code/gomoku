#pragma once
#include<iostream>
#include<mysql/mysql.h>
#include<sstream>
#include<memory>
#include<string>
#include<jsoncpp/json/json.h>
#include<fstream>
#include<pthread.h>
#include<string.h>

#define INF 0
#define DBG 1
#define ERR 2
#define NOW_LV 0

#define LOG(LV, format, ...)\
    do{\
    if(NOW_LV > LV) {break;}\
    time_t t = time(nullptr);\
    struct tm* ltm = localtime(&t);\
    char time_arr[32] = {0};\
    strftime(time_arr, 31, "%F %T", ltm);\
    fprintf(stdout, "[%lu %s %s:%d]" format "\n", (unsigned long)pthread_self(), time_arr, __FILE__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

class Mysql_Util
{
    MYSQL* _mq;

    void close()
    {
        if(_mq) mysql_close(_mq);
        _mq = nullptr;
    }
public:
    bool create_mysql(const char *host, const char *user, 
                    const char *passwd, const char *db, 
                    unsigned int port, const char *unix_socket, 
                    unsigned long clientflag)
    {
        _mq = mysql_init(nullptr);
        if(!_mq) { ERR_LOG("mysql init error: %s", mysql_error(_mq)); close(); return false; }
        _mq = mysql_real_connect(_mq, host, user, passwd, db, port, unix_socket, clientflag);
        if(!_mq) { ERR_LOG("connect error: %s", mysql_error(_mq)); close(); return false; }
        // int mysql_set_character_set(MYSQL *mysql, const char *csname)
        int back = mysql_set_character_set(_mq, "utf8");
        if(back != 0)  { ERR_LOG("set character error: %s", mysql_error(_mq)); close(); return false; }
        return true;
    }

    bool exec_mysql(const char* exec)
    {
        if(!_mq) { DBG_LOG("mysql closed."); return false; }
        if(!exec) { DBG_LOG("Invalid string."); return false; }
        if(mysql_query(_mq, exec) != 0)
        {
            DBG_LOG("statement execution error: %s", mysql_error(_mq));
            return false;
        }
        return true;
    }

    MYSQL_RES* result_mysql()
    {
        if(!_mq) { DBG_LOG("mysql closed."); return nullptr; }
        return mysql_store_result(_mq);
    }

    ~Mysql_Util()
    {
        close();
    }
};

class Json_Util
{
public:
    static bool serialize(const Json::Value &root, std::string &str)
    {
        Json::StreamWriterBuilder swb;
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
        std::stringstream ss;
        int ret = sw->write(root, &ss);
        if(ret < 0) { DBG_LOG("Json::StreamWriter::write: %s", strerror(errno)); return false; }
        str = ss.str();
        return true;
    }

    static bool unserialize(const std::string &str, Json::Value &root)
    {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, nullptr);
        if(!ret) { DBG_LOG("Json::CharReader::parse: %s", strerror(errno)); return false; }
        return true;
    }
};

class String_Util
{
public:
    static void str_split(const std::string& str, const std::string& inter, std::vector<std::string>& output)
    {
        size_t pos = 0;
        size_t gap = inter.size();
        while(pos < str.size())
        {
            size_t ret = str.find(inter, pos);
            if(ret == std::string::npos)
            {
                output.push_back(str.substr(pos));
                break;
            }
            if(ret == pos) { pos += gap; continue; }
            output.push_back(str.substr(pos, ret - pos));
            pos = ret + gap;
        }
    }
};

class File_Util
{
public:
    static bool read_file(const std::string& filename, std::string& buf)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if(!ifs.is_open())
        {
            ERR_LOG("file open fail.");
            return false;
        }
        ifs.seekg(0, ifs.end);
        size_t sz = ifs.tellg();
        ifs.seekg(0, ifs.beg);
        buf.resize(sz);
        ifs.read(&buf[0], sz);
        if(!ifs.good())
        {
            ERR_LOG("read file fail.");
            return false;
        }
        ifs.close();
        return true;
    }

    static bool write_file(const std::string& filename, std::string& buf)
    {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        if(!ofs.is_open())
        {
            ERR_LOG("file open fail.");
            return false;
        }
        ofs.write(buf.data(), buf.size());
        if(!ofs.good())
        {
            ERR_LOG("write file fail.");
            return false;
        }
        return true;
    }
};