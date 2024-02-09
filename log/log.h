#ifndef LOG_H
#define LOG_H
#include<iostream>
#include<stdio.h>
#include<string>
#include<fstream>
#include<queue>

#include"../lock/my_lock.h"
#include"../lock/my_sem.h"

using namespace std;

class Log{
public:
    static Log* getLog();

    void init(string file, bool asy = false);

    void write(string, int);//向日志文件写入数据

    static void* flush_log_thread(void*);//异步写时调用的函数

    void run();//异步线程运行函数

    void append(string, int);//向任务队列增加需要写的字符串
private:
    Log(){};
    ~Log();

private:
    string fileName;
    bool is_async;
    ofstream outputFile;
    My_lock que_mutex;      //互斥访问write_que的互斥量
    My_sem sem;             //同步生产者和消费者
    queue<pair<string, int>> write_que;
};

#endif