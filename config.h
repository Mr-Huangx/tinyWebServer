#ifndef CONFIG_H
#define CONFIG_H

#include<unistd.h>
#include<string>
#include<stdlib.h>

using namespace std;

class Config{
public:
    Config();
    ~Config(){};
    void init(int argc, char* argv[]);//初始化config实例

    //服务器端口号
    int PORT;

    //日志写入方式
    int LOGWrite;

    //触发组合模式
    int TRIGMode;

    //listenfd的触发模式
    int LISTENTrigmode;

    //如何关闭
    int OPT_LINGER;

    //sql连接池中连接数量
    int sql_num;

    //线程池中线程数量
    int thread_num;

    //是否关闭日志
    int close_log;

    //并发模式选择（半同步/半反应堆模型，半同步/半异步模型，领导者/追随者模型）
    int actor_model;

    //手动输入服务器ip地址
    const char* ip;
};


#endif
