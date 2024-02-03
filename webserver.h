#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unistd.h>
#include<string>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<assert.h>
#include<sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>

#include"config.h"


using namespace std;

class WebServer{
public:
    void init(string user, string passwd, string database,Config config);
    void eventListen();//创建listen socket同时进入循环监听状态

public:
    //链接数据库时的参数
    string user;
    string password;
    string database;
    Config config;
};


#endif
