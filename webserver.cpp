#include"webserver.h"

void WebServer::init(string user, string passwd, string database,Config config)
{
    user = user;
    passwd = passwd;
    database = database;
    config = config;
}

void webserver::eventListen(){
    //创建listen socket，使用TCP连接
    int listenfd = socket(PF_INET, STREAM, 0);
    assert(listenfd > 0);

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
    inet_pton(AF_INET, config.ip, &address.sin_addr);
    address.sin_port = htons(config.PORT);
}