#include "threadpool.h"

template<typename T>
My_lock ThreadPool<T>:: cout_mutex;


template<typename T>
ThreadPool<T>::ThreadPool(int actor_model, connection_pool* connPool, int thread_num, int max_requests):thread_num(thread_num), max_request_num(max_requests), threads(NULL), actor_model(actor_model),connPool(connPool)
{

    if(thread_num <= 0 || max_requests <= 0)
        throw std::exception();

    threads = new pthread_t[thread_num];

    if(!threads){
        //创建线程数组失败
        throw std::exception();
    }

    for(int i = 0; i < thread_num; i++){
        //实例化thread，并配置其回调函数
        if(pthread_create(threads + i, NULL, worker, this) != 0){
            //实例化thread失败
            delete[] threads;
            throw std::exception();
        }
        if(pthread_detach(threads[i])){
            //detach失败
            delete[] threads;
            throw std::exception();
        }
    }
}

//析构函数
template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] threads;
}


template<typename T>
bool ThreadPool<T>::append(T *request, int state){
    queue_mutex.lock();
    //先判断任务队列是否满
    if(task_queue.size() >= max_request_num){
        queue_mutex.unlock();
        return false;
    }

    //将http_conn的状态设置为传入的state状态，0为读，1为写
    request->state = state;
    task_queue.push_back(request);
    queue_mutex.unlock();

    //唤醒一个线程，进行处理
    queue_sem.post();
    return true;
}

template<typename T>
bool ThreadPool<T>::append_p(T *request){
    queue_mutex.lock();

    if(task_queue.size() >= max_request_num){
        queue_mutex.unlock();
        return false;
    }
    task_queue.push_back(request);
    queue_mutex.unlock();
    queue_sem.post();
    return true;
}

template<typename T>
void *ThreadPool<T>::worker(void *arg){
    //每个线程的拿到task后的回调函数
    ThreadPool *pool = (ThreadPool*)arg;
    pool->run();
    return pool;
}


template<typename T>
void ThreadPool<T>::run(){
    //一直尝试从任务队列中取出task，并处理task
    while(1){
        //先P信号量，如果有任务，则继续进行
        queue_sem.wait();

        //如果有任务，则先上锁，互斥访问task_queue
        queue_mutex.lock();

        if(task_queue.empty()){
            //如果任务队列为空，则重新尝试该任务，因为信号量的V操作可能产生伪唤醒
            queue_mutex.unlock();
            continue;
        }

        T *request = task_queue.front();
        task_queue.pop_front();

        //解锁任务队列互斥量
        queue_mutex.unlock();

        if(!request) continue;
        if(actor_model == 1){
            //如果是反应堆模型(reactor)
            if(request->state == 0){
                //读操作
                if(request->read_once()){
                    //一次性将数据读完
                    //由于读操作之后可能需要根据数据库判断用户是否存在，因此需要连接池的帮助
                    // request->imporv = 1;//好像没啥用
                    connectionRAII mysqlcon(&request->mysql, connPool);
                    request->process();
                }
                else{
                    //数据读取失败，怎么办呢？
                    cout_mutex.lock();
                    cout<<"数据处理失败"<<endl;
                    cout_mutex.unlock();
                }
                
            }
            else{
                //如果是写操作
                //写操作直接发送数据给客户端，所以不需要连接池帮助
                request->write();
            }
        }
        else{
            //如果是proactor模型
            //这个编写不对，因为这样的话，没有读取数据哇
            connectionRAII mysqlcon(&request->mysql, connPool);
            request->process();
        }
    }
}



