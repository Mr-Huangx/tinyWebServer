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
#include"utils/utils.h"
#include"./http/http_conn.h"
#include"./threadpool/threadpool.cpp"
#include"./CGImysql/sql_connection_pool.h"


using namespace std;

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数

class WebServer{
public:
    WebServer();
    ~WebServer();
    void init(string user, string passwd, string database,Config config);
    void init_connection_pool();//初始化连接池
    void init_threadPool();//初始化线程池
    void eventListen();//创建listen socket同时进入循环监听状态
    bool deal_client_connect();//listenfd监听到有新的连接到来，处理客户连接
    void deal_read_data(int sockfd);//处理sockfd读数据
    void deal_write_data(int sockfd);//处理sockfd写数据

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

    //记录所有http连接,以及处理，使得WebServer服务器不显得那么臃肿

    http_conn* users;
    //记录资源的路径
    char* source;

    //线程池
    ThreadPool<http_conn>* threadPool;//定义一个线程池，处理的任务类型是http_conn类型 

    //连接池
    connection_pool* connPool;  //定义一个连接池

    //工具箱,主要是对socket进行一些设置
    Utils utils;
};


#endif
