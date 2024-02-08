#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<list>
#include<pthread.h>
#include<exception>

#include"./lock/my_cond.h"
#include"./lock/my_lock.h"
#include"./lock/my_sem.h"
#include"../CGImysql/sql_connection_pool.h"


template<typename T>
class ThreadPool{
public:
    ThreadPool(int actor_model, connection_pool* connPool, int thread_num=8, int max_requests=10000);
    ~ThreadPool();

    bool append(T *request, int state);//增加reactor的任务

    bool append_p(T *request);//增加proactor的任务
    
private:
    static void *worker(void* arg); ////每个线程的拿到task后的回调函数
    void run();//线程池不断运行，获得任务队列中的任务进行处理
    
    


private:
    int thread_num;         //线程池中，线程数量
    int max_request_num;    //任务队列中允许的最大请求数
    pthread_t *threads;     //线程数组，大小为thread_num
    list<T *> task_queue;   //任务队列
    My_lock queue_mutex;    //任务队列互斥量，互斥使用任务队列
    My_sem queue_sem;       //任务队列信号量，用于消费者-生产者模型

    int actor_model;        //I/O模型[reactore, proactor]

    //连接池
    connection_pool* connPool; //连接池，给线程用于连接数据库使用

    static My_lock cout_mutex; //用于互斥访问标准输出缓冲区
    
};

#endif