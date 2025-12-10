#pragma once

// server 和 client 共有的部分
enum class MsgId
{
    LOGIN_MSG = 1,
    REG_MSG,
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,
    LOG_OUT_MSG,

    LOGIN_MSG_ACK,
    REG_MSG_ACK, 

    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,
};