#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#inlcude "../timer/lst_timer.h"
class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //从内核事件表中移除对应的文件描述符（弃用，由cb_func完成任务）
    void removefd(int epollfd, int fd);

    //如果事件开启了EPOLLONESHOT，则处理完之后需要重新将其设置为EPOLLONESHOT
    void modfd(int epollfd, int fd, int ev, int TRIGMode);

    //信号处理函数，主要是向管道写入信号
    static void sig_handler(int sig);

    //设置信号处理函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();


public:
    static int epollfd;
    static int *pipefd; //用pipefd通知主循环，定时处理链表上的定时任务
    int TIMESLOT;
    sort_timer_lst timer_list;
};


#endif