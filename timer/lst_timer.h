/*
    基于升序链表定时器，来管理所有定时器
    采用定时器处理非活跃的链接--参考《Linux高性能服务器编程》P200
    利用定时器alarm周期性地触发SIGALRM信号，并利用管道通知主循环执行定时器链表上的定时任务--关闭非活跃的链接
*/

#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;   //前向声明，因为我们接下来定义的用户数据结构用到了这个类，同时这个类又包含客户数据

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer* timer;  //只能使用指针，因为编译器还不知道这个类
    char buf[BUFFER_SIZE];
}

//定时器类
class util_timer{
public:
    util_timer():prev(nullptr), next(nullptr){}

public:
    time_t expire;//任务的超时时间，这里使用绝对时间
    void (*cb_func)(client_data*);//任务回调函数
    client_data* user_data;
    util_timer* prev;
    util_timer* next;
}

//定时器链表。升序、双向链表，带有头结点和尾节点
class sort_timer_lst
{
public:
    sort_timer_lst() : head(nullptr), tail(nullptr){}
    /*销毁链表时，删除其中所有的定时器*/
    ~sort_timer_lst();

    void add_timer(util_timer*);//将目标定时器timer添加到链表中
    void adjust_timer(util_timer* timer);//当某个定时器任务发生变化是，对应调整其在链表中的位置
    void del_timer(util_timer*);//将目标定时器timer从链表中删除
    void tick();//SIGALRM信号每次触发就在其信号处理函数中执行一次tick函数，处理链表上到期的任务

private:
    void add_timer(util_timer* , util_timer* lst_head);//重载函数，被公有add_timer和adjust_timer函数调用，该函数表示把目标定时器timer添加到节点lst_head之后的部分链表中

private:
    util_timer* head;
    util_timer* tail;
}




#endif