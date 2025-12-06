#include<iostream>

#include<sstream>

#include<memory>

#include<string>

#include<jsoncpp/json/json.h>

bool serialize(const Json::Value &root, std::string &str)
{
    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    std::stringstream ss;
    int ret = sw->write(root, &ss);
    if(ret < 0) { perror("Json::StreamWriter::write"); return false; }
    str = ss.str();
    return true;
}

bool unserialize(const std::string &str, Json::Value &root)
{
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, nullptr);
    if(!ret) { perror("Json::CharReader::parse"); return false; }
    return true;
}

int main()
{
    Json::Value va1;
    va1["name"] = "童宏旭";
    va1["age"] = 20;
    va1["socre"].append(99);
    va1["socre"].append(98);
    va1["socre"].append(97);

    std::string str;
    serialize(va1, str);
    std::cout << str << std::endl;

    Json::Value va2;
    unserialize(str, va2);
    std::cout << "name: " << va2["name"].asString() << std::endl;
    std::cout << "age: " << va2["age"].asInt() << std::endl;
    std::cout << "socre: " << va2["socre"][0].asInt() << std::endl;
    std::cout << "socre: " << va2["socre"][1].asInt() << std::endl;
    std::cout << "socre: " << va2["socre"][2].asInt() << std::endl;
    return 0;
}