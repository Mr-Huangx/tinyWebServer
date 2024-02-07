#include"config.h"
#include"webserver.h"
#include<iostream>
using namespace std;

int main(int argc, char *argv[]){
    //输入登陆mysql的账户和密码
    string user = "root";
    string passwd = "000000";

    //使用的数据库名
    string database = "tinyWebServer";

    //命令行参数解析
    Config config;
    config.init(argc, argv);
    cout<<"config init ok"<<endl;

    WebServer server;


    //初始化服务器的各项参数
    server.init(user, passwd, database, config);
    cout<<"webserver init ok"<<endl;

    //初始化数据库
    server.init_connection_pool();
    cout<<"connection pool init ok"<<endl;

    //初始化线程池
    server.init_threadPool();
    cout<<"threadpool init ok"<<endl;

    //开始运行
    server.eventListen();

    return 0;
}