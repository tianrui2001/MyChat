#pragma once

// server 和 client 共有的部分
enum class MsgId
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    REG_MSG, 
    REG_MSG_ACK, 
};