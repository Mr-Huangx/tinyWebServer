#ifndef MY_SEM_H
#define MY_SEM_H

#include <exception>
#include <semaphore.h>

class My_sem{
public:
    My_sem(){
        //sem_init 初始化信号量，成功返回0；
        //默认信号量初始值为0
        if(sem_init(&sem, 0, 0) != 0){
            throw std::exception();
        }
    }

    My_sem(int val){
        //val为信号量的初始值
        if(sem_init(&sem , 0, val) != 0){
            throw std::exception();
        }
    }

    ~My_sem(){
        sem_destroy(&sem);
    }

    bool wait(){
        return sem_wait(&sem) == 0;
    }

    bool post(){
        return sem_post(&sem) == 0;
    }


private:
    sem_t sem;
};


#endif