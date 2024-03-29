#include"config.h"

Config::Config(){
    //初始化所有参数为默认值
    PORT = 15433;

    //日志写入方式,默认为同步
    LOGWrite = 0;

    //触发组合模式,默认为listenfd LT + connfd LT
    TRIGMode = 0;

    //listenfd的触发模式，默认为LT
    LISTENTrigmode = 0;

    //connfd的触发模式，默认为LT
    CONNTrigmode = 0;

    //是否采用优雅关闭socket，默认不使用
    OPT_LINGER = 0;

    //sql连接池中连接数量，默认4个
    sql_num = 4;

    //线程池中线程数量，默认4个
    thread_num = 4;

    //是否关闭日志，默认不关闭
    close_log = 0;

    // I/O模式选择
    actor_model = 1;

    //ip地址,默认为空
    ip = "localhost";
}

void Config::init(int argc, char* argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:i:";
    //getop(argc, argv, str)根据str获取对应选项的参数
    while((opt = getopt(argc, argv, str)) != -1){
        switch (opt)
        {
        case 'p':
        {   
            //atoi()将字符串转换成int
            //optarg对应当前选项的参数
            PORT = atoi(optarg);

            //也可以optint使用
            // PORT = atoi(argv[optind-1]);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            TRIGMode = atoi(optarg);
            deal_trigmode();
            break;
        }
        case 'o':
        {
            OPT_LINGER = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'c':
        {
            close_log = atoi(optarg);
            break;
        }
        case 'a':
        {
            actor_model = atoi(optarg);
            break;
        }
        case 'i':
        {
            ip = string(optarg);
            break;
        }
        default:
            break;
        }
    }

    // 测试解析结果
    test_content();
}
//将TRIGMode拆解为LISTENTrigmode、 CONNTrigmode
void Config::deal_trigmode(){
    switch(TRIGMode){
        case 0:
        {
            //LT+LT
            LISTENTrigmode = 0;
            CONNTrigmode = 0;
            break;
        }
        case 1:
        {
            //LT+ET
            LISTENTrigmode = 0;
            CONNTrigmode = 1;
            break;
        }
        case 2:
        {
            //ET+LT
            LISTENTrigmode = 1;
            CONNTrigmode = 0;
            break;
        }
        case 3:
        {
            //ET+ET
            LISTENTrigmode = 1;
            CONNTrigmode = 1;
            break;
        }
    }
}

void Config::test_content(){
    cout<<"port: "<<PORT<<endl;
    cout<<"LOGWrite: "<<LOGWrite<<endl;
    cout<<"TRIGMode: "<<TRIGMode<<endl;
    cout<<"LISTENTrigmode: "<<LISTENTrigmode<<endl;
    cout<<"OPT_LINGER:" <<OPT_LINGER<<endl;
    cout<<"sql_num: "<<sql_num<<endl;
    cout<<"thread_num: "<<thread_num<<endl;
    cout<<"close_log: "<<close_log<<endl;
    cout<<"actor_model: "<<actor_model<<endl;
    cout<<"ip: "<<ip<<endl;
}