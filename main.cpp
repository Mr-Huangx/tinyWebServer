#include"config.h"

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
    server.init(user, passwd, database, config.PORT, config.LOGWrite,
                config.OPT_LINGER, config.TRIGMode,config.sql_num,
                config.thread_num, config.close_log, config.actor_model);

    

    return 0;
}