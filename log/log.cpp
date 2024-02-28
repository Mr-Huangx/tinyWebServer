#include"log.h"

Log::~Log(){
    outputFile.close();
}

void Log::init(string file, bool asy){
    //is_async==true表示异步写
    fileName = file;
    is_async = asy;

    outputFile.open(file);
    if(outputFile.is_open()){
        cout<<"文件已经打开！"<<endl;
    }
    else{
        cout<<"文件打开失败\n";
        return;
    }

    //如果是异步写，还需要创建一个单独的线程进行操作
    if(is_async){
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, this);
    }

}

Log* Log::getLog(){
    static Log log;
    return &log;
}

void Log::write(string str, int level){
    //开始写
    switch(level){
        case 0:
            str = "info:\n" + str + "\n";
            break;

        case 1:
            str = "bug:\n" + str + "\n";
            break;
    }
    
    outputFile<<str;

}

void* Log::flush_log_thread(void* arg){
    Log* log = (Log*)arg;
    log->run();
    return log;
}

void Log::run(){
    while(1){
        sem.wait();
        que_mutex.lock();

        if(!write_que.size()){
            que_mutex.unlock();
            continue;
        }
        string str = write_que.front().first;
        int level = write_que.front().second;

        write_que.pop();
        write(str, level);
        que_mutex.unlock();
    }
}

void Log::append(string str, int level = 0){
    que_mutex.lock();
    write_que.push(pair<string, int>(str, level));
    que_mutex.unlock();
    sem.post();
}