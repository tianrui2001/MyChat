#pragma once

#include "user.hpp"

#include <vector>
using namespace std;

class FriendModel
{
public:
    void insert(int userid, int friendid);

    vector<User> query(int userid);
};