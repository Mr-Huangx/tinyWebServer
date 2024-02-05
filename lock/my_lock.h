#ifndef MY_LOCK_H
#define MY_LOCK_H

#include <pthread.h>
#include <exception>

//将互斥量装成RAII资源，减少使用过程中手动分配和释放该资源
class My_lock
{
public:
    My_lock(){
        //pthread_mutex_init( &m_mutex, NULL)初始化普通锁
        //成功返回0
        if(pthread_mutex_init(&mutex, NULL) != 0){
            throw std::exception();
        }
    }

    ~My_lock(){
        pthread_mutex_destroy(&mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&mutex) == 0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&mutex) == 0;
    }

    pthread_mutex_t* get(){
        return &mutex;
    }


private:
    pthread_mutex_t mutex;
};

#endif