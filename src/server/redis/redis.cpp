#include "redis.hpp"

#include <string>
#include <iostream>

using namespace std;

static string redis_ip = "127.0.0.1";
static int redis_port = 6379;

Redis::Redis()
    :_publish_context(nullptr),
    _subscribe_context(nullptr){}

Redis::~Redis(){
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }

    if(_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}

bool Redis::connect(){
    _publish_context = redisConnect(redis_ip.c_str(), redis_port);
    if(_publish_context == nullptr || _publish_context->err){
       cerr << "connect redis server failed!" << endl;
         return false; 
    }

    _subscribe_context = redisConnect(redis_ip.c_str(), redis_port);
    if(_subscribe_context == nullptr || _subscribe_context->err){
         cerr << "connect redis server failed!" << endl;
            return false;
    }

    // 在单独的线程中监听通道上的事件，有消息就给业务层上报
    thread t(&Redis::observe_channel_message, this);
    t.detach(); // 分离线程

    cout << "connect redis server success!" << endl;
    return true;
}

bool Redis::publish(int channel, string message){
    redisReply *reply = (redisReply *)redisCommand(this->_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr){
        cerr << "publish command failed!" << endl;
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel){
    // 不用 redisCommand ，是因为 redisCommand 里面调用的 redisGetReply 会阻塞当前的线程
    // redisCommand调用了： redisAppendCommand 把命令写入本地发送缓冲区
    //                     redisBufferWrite 把本地缓冲区的命令通过网络发送出去
    //                     redisGetReply 阻塞等待redis server响应消息
    if(redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR){
        cerr << "subscribe command failed!" << endl;
        return false;
    }

    // 统一把命令发送到 redis 服务器上
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR){
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel){
    if(redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR){
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    // 统一把命令发送到 redis 服务器上
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context, &done) == REDIS_ERR){
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

void Redis::observe_channel_message(){
    redisReply *reply = nullptr;
    while(redisGetReply(this->_subscribe_context, (void **)&reply) == REDIS_OK){
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            // 给业务层上报订阅的消息
            // element[1]：通道号，； element[2]：消息内容
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << "=======================observe_channel_message quit!=======================" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn){
    _notify_message_handler = fn;
}