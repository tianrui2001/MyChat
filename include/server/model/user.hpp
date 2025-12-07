#pragma once

#include <string>

using namespace std;

// 匹配user表的ORM类
class User
{
public:
    User(int id = -1, string name = "", string password = "", string state = "offline")
        : id(id), name(name), password(password), state(state)
    {}

    void setId(int id_) { id = id_;}
    void setName(string name_) { name = name_;}
    void setPassword(string password_) { password = password_;}
    void setState(string state_) { state = state_;}
    int getId() { return id;}
    string getName() { return name;}
    string getPassword() { return password;}
    string getState() { return state;}
private:
    int id;
    string name;
    string password;
    string state;
};