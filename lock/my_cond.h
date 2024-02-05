#ifndef MY_COND_H
#define MY_COND_H

#include <pthread.h>
#include <exception>

//将条件变量封装成RAII资源，减少使用过程中手动分配和释放该资源
class My_cond{
public:
    My_cond(){
        //初始化普通条件变量
        //条件变量和信号量的区别：
        //    条件变量，主要用于线程间的通信
        //    信号量，主要用于进程间的通信
        if (pthread_cond_init(&cond, NULL) != 0)
        {
            throw std::exception();
        }
    };

    ~My_cond(){
        pthread_cond_destroy(&cond);
    }

    //条件变量wait操作，需要传入一个锁
    bool wait(pthread_mutex_t *mutex){
        return pthread_cond_wait(&cond, mutex) == 0;
    }

    bool signal(){
        return pthread_cond_signal(&cond) == 0;
    }

    bool broadcast(){
        return pthread_cond_broadcast(&cond) == 0;
    }

private:
    //私有条件变量
    pthread_cond_t cond;
};

#endif