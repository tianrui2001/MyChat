#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis {
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向 redis 指定通道发布消息
    bool publish(int channel, string message);

    // 向 redis 指定通道订阅消息
    bool subscribe(int channel);

    // 向 redis 指定通道取消订阅消息
    bool unsubscribe(int channel);

    // 观察者线程，专门用来接收订阅的消息
    void observe_channel_message();

    // 初始化回调操作，给上层使用的
    void init_notify_handler(function<void(int, string)> fn);


private:
    // hiredis的上下文对象， 负责发布消息
    redisContext *_publish_context;

    // hiredis的上下文对象， 负责订阅消息
    redisContext *_subscribe_context;

    // 回调操作，订阅消息，给上层上报
    function<void(int, string)> _notify_message_handler;
};