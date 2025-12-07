#pragma once

#include <vector>

#include "group.hpp"

using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    void addGroup(int userid, int groupid, string role);

    // 查询用户所在的群组信息
    vector<Group> queryGroups(int userid);

    // 查询群组成员信息
    vector<int> queryGroupUsers(int userid, int groupid);

};
