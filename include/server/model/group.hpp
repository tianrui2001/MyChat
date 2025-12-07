#pragma once

#include <string>
#include <vector>

#include "groupuser.hpp"

using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) { this->id = id; }
    void setName(const string &name) { this->name = name; }
    void setDesc(const string &desc) { this->desc = desc; }

    int getId() { return id; }
    string getName() { return name; }
    string getDesc() { return desc; }
    vector<GroupUser>& getUsers() { return user; }


private:
    int id;
    string name;
    string desc;
    vector<GroupUser> user; // 群成员列表
};