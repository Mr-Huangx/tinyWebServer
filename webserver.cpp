#include "webserver.h"


WebServer::WebServer(){
    //初始化users
    users = new http_conn[MAX_FD];

    //拼接资源路径，即目录中root目录的路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    source = (char*)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(source, server_path);
    strcat(source, root);

    //初始化定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer()
{
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete threadPool;
}

void WebServer::init(string user, string passwd, string database,Config config)
{
    this->user = user;
    this->password = passwd;
    this->database = database;
    this->config = config;
}

void WebServer::init_log(string file, bool asy)
{
    //初始化日志文件
    log = Log::getLog();
    log->init(file, asy);
}

void WebServer::init_connection_pool()
{
    //初始化连接池
    connPool = connection_pool::getInstance();//获取单例

    //输出所有参数
    cout<<"config.ip :"<<config.ip<<endl;
    cout<<"user :"<<user<<endl;
    cout<<"password :"<<password<<endl;
    cout<<"database :"<<database<<endl;
    cout<<"config.sql_num :"<<config.sql_num<<endl;
    connPool->init(config.ip, user, password, database, 3306, config.sql_num);
    //初始化http_con类中静态成员变量users，记录数据库中的所有用户名和用户密码
    users->initmysql_result(connPool);
}

void WebServer::init_threadPool()
{
    //初始化线程池
    threadPool = new ThreadPool<http_conn>(config.actor_model, connPool, config.thread_num);
}


void WebServer::eventListen(){
    //创建listen socket，使用TCP连接
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);
    this->listenfd = listenfd;

    //对于listenfd的关闭，采用默认方式，即如果有数据，则交给TCP模块负责
    if(0 == config.OPT_LINGER){
        //采用默认方式，不需要设置listenfd
    }
    else if(1 == config.OPT_LINGER){
        //close行为由自己控制，根据close返回值和errno是否为EWOULDBLOCK确定
        struct linger option_value = {1, 1};
        setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &option_value, sizeof(option_value));
    }

    //配置server的IP地址，端口号
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    inet_pton(AF_INET, config.ip.data(), &address.sin_addr);
    address.sin_port = htons(config.PORT);

    //设置服务器使用的端口号为可立即重用
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    //将listenfd和address进行绑定
    assert(bind(listenfd, (struct sockaddr*)&address, sizeof(address)) >= 0);
    
    //设置监听队列最大长度
    assert(listen(listenfd, 5) >= 0);

    utils.init(TIMESLOT);

    //创建epoll句柄
    epollfd = epoll_create(5);
    Utils::epollfd = epollfd;
    http_conn::epollfd = epollfd;

    assert(epollfd != -1);

    //创建epoll事件表，用于接收有事件发生的epoll_event
    epoll_event events[MAX_EVENT_NUMBER];

    //将listenfd加入到epollfd中,同时不采用ONESHOT参数，因为这个sockfd上的任务并不会被其他线程所拿到
    utils.addfd(epollfd, listenfd, false, config.LISTENTrigmode);
    
    //创建双工pipefd，用于当前循环程序和链表通信
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    utils.setnonblocking(pipefd[1]);
    utils.addfd(epollfd, pipefd[0], false, 0);
    //初始化工具包中的管道
    Utils::pipefd = pipefd;

    //因为后面会一直在while循环里进行工作，我们需要让while循环知道shell里面有什么信号发出，应该做什么
    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);
    
    //开启定时任务，不过这个信号会被系统捕捉，然后使用管道，写到pipefd[1]中，由pipefd[0]接收
    alarm(TIMESLOT);

    //debug listenfd
    cout<<"listen fd:"<<listenfd<<endl;
    
    //进入循环监听任务
    bool timeout = false;
    bool stop_server = false;
    while(!stop_server){
        //使用epoll_wait进行监听，传入-1一直阻塞，直到检测到有事件为止
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        if(number < 0 && errno != EINTR){
            //出错

            break;
        }

        for(int i = 0; i < number; i++){
            //依次遍历各个事件进行处理
            int sockfd = events[i].data.fd;

            //判断文件描述符类型
            if(sockfd == listenfd){
                //如果是listenfd有事件发生，即有新的客户连接
                bool flag = deal_client_connect();
                //输出日志
                // cout<<"listenfd检测到有新的连接到来，并建立连接，客户端socket"<<sockfd<<endl;
                if(flag == false){
                    cout<<"listenfd在处理新的连接时出错了"<<endl;
                    continue;
                }
            }
            //处理信号
            else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){
                deal_signal(timeout, stop_server);
            }
            //处理客户端发来数据
            else if(events[i].events & EPOLLIN){
                cout<<"read task:\n";
                cout<<"epoll_wait发现有客户端socket:"<< sockfd <<"发来请求，将请求放入队列中\n";
                deal_read_data(sockfd);
            }
            //处理发送数据
            else if(events[i].events & EPOLLOUT){
                cout<<"write task:\n";
                cout<<"epoll_wait socket:" << sockfd << "需要发送数据，将请求放入队列中\n";
                deal_write_data(sockfd);
            }
            //1、检测到TCP连接被对方关闭
            //2、检测到错误
            //3、挂起（管道的写端被关闭)
            //综上：只要出现异常，则关闭socket
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                util_timer* timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            } 
        }

        //在将当前socket上的事件处理完后，再处理定时任务（即清理不活跃的连接）
        if(timeout){
                //处理当前不活跃的socket
                
                utils.timer_handler();
                timeout = false;
            }
    }
    
}

//使用timer来包装socket，并进行管理
void WebServer::timer(int connfd, struct sockaddr_in client_address){
    //将http记录到users数组中
    users[connfd].init(connfd, client_address, source, config.CONNTrigmode, config.close_log, user, password, database);
    
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3*TIMESLOT;

    users_timer[connfd].timer = timer;
    utils.timer_list.add_timer(timer);
    

}

void WebServer::deal_timer(util_timer *timer, int sockfd){
    //将当前timer中的socket关闭，并且从升序链表中删除
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.timer_list.del_timer(timer);
    }
}
void WebServer::deal_signal(bool& timeout, bool& stop_server){
    char signals[1024];
    int ret = recv(pipefd[0], signals, sizeof(signals), 0);
    if(ret == -1){
        return;
    }
    else if(ret == 0){
        return;
    }
    else{
        for(int i= 0; i > ret; i++){
            switch (signals[i])
            {
                //timeout变量标记有定时任务需要处理，但不立即处理定时任务，因为定时任务的优先级不是很高，我们优先处理其他更加重要的任务
            case SIGALRM:
            {
                timeout = true;
                break;
            }
                //终端发送的指令是终止服务，则关闭服务器
            case SIGTERM:
            {
                stop_server = true;
            }
            
            
            default:
                break;
            }
        }
    }
}

bool WebServer::deal_client_connect(){
    //处理新的客户连接
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    
    if(config.LISTENTrigmode == 0){
        //如果listenfd采用的是LT模式
        int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
        if(connfd < 0){
            //建立连接失败
            return false;
        }

        if(http_conn::user_count >= MAX_FD){
            //http连接数量已经到达系统最大文件描述符数量，现在不能建立新的连接
            return false;
        }
        // 第一版，不适用timer，直接使用socket进行管理
        // //将connfd加入到epollfd中
        // utils.addfd(epollfd, connfd, true, config.CONNTrigmode);

        // //将http记录到users数组中
        // users[connfd].init(connfd, client_address, source, config.CONNTrigmode, config.close_log, user, password, database);
        
        // http_conn::user_count++;
        // 第一版结束

        //第二版，使用timer类进行管理和封装
        timer(connfd, client_address);
    }
    else{
        //如果listenfd采用的是ET模式
        while(1){
            int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
            if(connfd < 0){
                //出错
                break;
            }

            if(http_conn::user_count >= MAX_FD){
            //http连接数量已经到达系统最大文件描述符数量，现在不能建立新的连接
                return false;
            }
            //第一版，不适用timer
            // //将connfd加入到epollfd中
            // utils.addfd(epollfd, connfd, true, config.CONNTrigmode);

            // //将http记录到users数组中
            // users[connfd].init(connfd, client_address, source, config.CONNTrigmode, config.close_log, user, password, database);
            // http_conn::user_count++;
            // 第一版结束

            //第二版
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

//处理客户端发来的数据
void WebServer::deal_read_data(int sockfd){
    //先拿到当前socket所对应的timer
    util_timer *timer = users_timer[sockfd].timer;

    //根据I/O模型，进行不同操作
    //reactor模型
    if(config.actor_model == 1){
        if(timer){
            //先将对应timer中的expire进行更新，因为他活跃了一次
            refresh_timer(timer);
        }

        //监听到读事件，将事件放入请求队列中，让逻辑处理单元进行处理
        while(!threadPool->append(users + sockfd, 0)){
            //如果加入失败，则可能是任务太多等待几秒再继续
            printf("sockfd:%d加入任务队列失败\n", sockfd);
        }
        cout<<"deal_read_data函数加入任务成功\n";

        while(true){
            if(users[sockfd].improv == 1){
                users[sockfd].improv = 0;
                break;
            }
        }
        cout<<"deal_read_data函数退出\n";
    }
    else{
        //proactor模型，通知就绪事件
        if(users[sockfd].read_once()){
            //由主进程一次性将数据处理完，然后通知程序进行后续操作
            while(!threadPool->append_p(users+sockfd)){
                //如果加入失败，则可能是任务太多等待几秒再继续
                printf("sockfd:%d加入任务队列失败\n", sockfd);
            }
            if(timer){
                refresh_timer(timer);
            }
        }
        else{
            //读取失败，则关闭连接
            deal_timer(timer, sockfd);
        }
    }
}

//更新当前timer中的expire时间
void WebServer::refresh_timer(util_timer* timer){
    time_t cur = time(NULL);
    timer->expire = cur + 3*TIMESLOT;
    utils.timer_list.adjust_timer(timer);//在管理的升序链表中，更新timer的位置
}

//处理sockfd需要发送数据
void WebServer::deal_write_data(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;

    if (1 == config.actor_model)
    {
        if(timer){
            refresh_timer(timer);
        }
        //reactor模式
        while( !threadPool->append(users + sockfd, 1)){
            //如果加入失败，则可能是任务太多等待几秒再继续
        }
        cout<<"deal_write_data函数加入写任务成功\n";
    
        while(true){
            if(1 == users[sockfd].improv){
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if(users[sockfd].write()){
            //proactor模式，只需要向软件通知完成即可，不需要处理
            //但是需要刷新timer的expire
            if(timer) refresh_timer(timer);
        }
        else deal_timer(timer, sockfd);
    }    
}

void cb_func(client_data* user_data)
{
    epoll_ctl(Utils::epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::user_count--;
}
