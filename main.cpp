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

    WebServer server;

    //初始化服务器的各项参数
    server.init(user, passwd, database, config);

    

    return 0;
}