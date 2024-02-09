#include"config.h"
#include"webserver.h"



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

    //构造函数会初始化server中root目录的路径
    WebServer server;


    //初始化服务器的各项参数
    //初始化日志文件
    server.init_log("log.txt", false);


    //初始化server中连接mysql的user、passwd、database、config成员变量
    server.init(user, passwd, database, config);
    cout<<"webserver init ok"<<endl;

    //初始化数据库
    //初始化连接池中mysql的ip地址，登录mysql服务器的账户、密码、数据库名、端口号、线程池数量
    //初始化连接池中所有的用户
    server.init_connection_pool();
    cout<<"connection pool init ok"<<endl;

    //初始化线程池
    server.init_threadPool();
    cout<<"threadpool init ok"<<endl;

    //开始运行
    server.eventListen();

    return 0;
}