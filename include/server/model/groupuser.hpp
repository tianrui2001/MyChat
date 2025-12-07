#pragma once

#include "user.hpp"

#include <string>

using namespace std;

class GroupUser: public User
{
public:
    void setRole(string role)  { this->role = role; }
    string getRole() { return role; }

private:
    string role; // 角色：管理员、成员
};