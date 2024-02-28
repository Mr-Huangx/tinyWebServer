#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unistd.h>
#include<string>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<assert.h>
#include<sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>

#include"config.h"
#include"./utils/utils.h"
#include"./http/http_conn.h"
#include"./threadpool/threadpool.cpp"
#include"./CGImysql/sql_connection_pool.h"
#include"./log/log.h"
#include"./timer/lst_timer.h"


using namespace std;

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时时间
class WebServer{
public:
    WebServer();
    ~WebServer();
    void init(string user, string passwd, string database,Config config);
    void init_log(string file, bool asy);//初始化日志文件
    void init_connection_pool();//初始化连接池
    void init_threadPool();//初始化线程池
    void eventListen();//创建listen socket同时进入循环监听状态
    void timer(int connfd, struct sockaddr_in client_address);//使用timer来创建管理socket类
    bool deal_client_connect();//listenfd监听到有新的连接到来，处理客户连接
    void deal_read_data(int sockfd);//处理sockfd读数据
    void deal_write_data(int sockfd);//处理sockfd写数据
    void deal_timer(util_timer *timer, int sockfd);//移除定时器，并关闭socket
    void deal_signal(bool& timeout, bool& stop_server);//处理信号
    void refresh_timer(util_timer* timer);//更新当前timer中的expire

public:
    //链接数据库时的参数
    string user;
    string password;
    string database;
    Config config;

    //listenfd
    int listenfd;

    //epoll参数
    int epollfd;

    //pipefd用于当前进程和子线程进行通信
    int pipefd[2];

    //记录log
    Log* log;

    //记录所有http连接,以及处理，使得WebServer服务器不显得那么臃肿
    http_conn* users;

    //记录资源的路径
    char* source;

    //线程池
    ThreadPool<http_conn>* threadPool;//定义一个线程池，处理的任务类型是http_conn类型 

    //连接池
    connection_pool* connPool;  //定义一个连接池

    //工具箱,主要是对socket进行一些设置，以及管理的timer链表
    Utils utils;

    //定时器相关
    client_data *users_timer;//记录所有的timer

};

//定时器回调函数，删除socket上的注册时间，从内核时间表中删除对应的文件描述符
void cb_func(client_data* user_data);

#endif
