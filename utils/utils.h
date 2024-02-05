#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(){};

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //从内核事件表中移除对应的文件描述符
    void removefd(int epollfd, int fd);

    //如果事件开启了EPOLLONESHOT，则处理完之后需要重新将其设置为EPOLLONESHOT
    void modfd(int epollfd, int fd, int ev, int TRIGMode);


public:
    static int epollfd;
};

#endif