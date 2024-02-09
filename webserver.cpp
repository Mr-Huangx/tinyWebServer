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

    //创建epoll句柄
    epollfd = epoll_create(5);
    Utils::epollfd = epollfd;
    http_conn::epollfd = epollfd;

    assert(epollfd != -1);

    //创建epoll事件表，用于接收有事件发生的epoll_event
    epoll_event events[MAX_EVENT_NUMBER];

    //将listenfd加入到epollfd中,同时不采用ONESHOT参数，因为这个sockfd上的任务并不会被其他线程所拿到
    utils.addfd(epollfd, listenfd, false, config.LISTENTrigmode);
        
    //debug listenfd
    cout<<"listen fd:"<<listenfd<<endl;
    
    //进入循环监听任务
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
                cout<<"listenfd检测到有新的连接到来，并建立连接，客户端socket"<<sockfd<<endl;
                if(flag == false){
                    cout<<"listenfd在处理新的连接时出错了"<<endl;
                    continue;
                }
            }
            //处理客户端发来数据
            else if(events[i].events & EPOLLIN){
                cout<<"epoll_wait发现有客户端发来请求，将请求放入队列中，等待处理：\n";
                deal_read_data(sockfd);
            }
            //处理发送数据
            else if(events[i].events & EPOLLOUT){
                cout<<"epoll_wait发现有socket需要发送数据，将请求放入队列中，等待处理:\n";
                deal_write_data(sockfd);
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

        //将connfd加入到epollfd中
        utils.addfd(epollfd, connfd, true, config.CONNTrigmode);

        //将http记录到users数组中
        users[connfd].init(connfd, client_address, source, config.CONNTrigmode, config.close_log, user, password, database);
        
        http_conn::user_count++;
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
            //将connfd加入到epollfd中
            utils.addfd(epollfd, connfd, true, config.CONNTrigmode);

            //将http记录到users数组中
            users[connfd].init(connfd, client_address, source, config.CONNTrigmode, config.close_log, user, password, database);
            http_conn::user_count++;
        }
        return false;
    }
    return true;
}

//处理客户端发来的数据
void WebServer::deal_read_data(int sockfd){
    //根据I/O模型，进行不同操作
    //reactor模型
    if(config.actor_model == 1){
        //监听到读事件，将事件放入请求队列中，让逻辑处理单元进行处理
        printf("deal_read_data函数处理时的sockfd:%d\n", sockfd);
        while(!threadPool->append(users + sockfd, 0)){
            //如果加入失败，则可能是任务太多等待几秒再继续
            printf("sockfd:%d加入任务队列失败\n", sockfd);
        }

    }
    else{
        //proactor模型，通知就绪事件
        if(users[sockfd].read_once()){
            //由主进程一次性将数据处理完，然后通知程序进行后续操作
            printf("deal_read_data函数处理时的sockfd:%d\n", sockfd);
            while(!threadPool->append_p(users+sockfd)){
                //如果加入失败，则可能是任务太多等待几秒再继续
                printf("sockfd:%d加入任务队列失败\n", sockfd);
            }
        }
    }
}

//处理sockfd需要发送数据
void WebServer::deal_write_data(int sockfd){
    
    if (1 == config.actor_model)
    {
        //reactor模式
        while( !threadPool->append(users + sockfd, 1)){
            //如果加入失败，则可能是任务太多等待几秒再继续
        }
    }
    else
    {
        //proactor
        if(users[sockfd].write()){
            //proactor模式，只需要向软件通知完成即可，不需要处理
        }
    }    
}
