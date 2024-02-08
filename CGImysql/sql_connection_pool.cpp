#include"sql_connection_pool.h"

connection_pool::connection_pool(){
    //构造函数中初始化变量
    usedConn = 0;
    freeConn = 0;
}

connection_pool::~connection_pool()
{
	//销毁连接池
    //互斥访问连接池
    pool_mutex.lock();

    if(!connList.empty())
    {
        list<MYSQL*>::iterator it;
        for(it = connList.begin(); it!= connList.end(); it++){
            MYSQL* con = *it;
            mysql_close(con);
        }

        usedConn = 0;
        freeConn = 0;
        connList.clear();
    }
    pool_mutex.unlock();
}

connection_pool* connection_pool::getInstance(){
    //单例模式，获取样例
    //采用局部静态变量的懒汉模式
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(string url, string user, string password, string database, int port, int sql_num)
{
    this->url = url;
    this->user = user;
    this->password = password;
    this->port = port;
    this->databasename = database;
    this->maxConn = sql_num;

    //根据sql_num的值，创建对应个mysql连接，并加入到connList中
    for(int i = 0; i < sql_num; i++){
        MYSQL* con = NULL;
        con = mysql_init(con);

        if(con == NULL){
            //创建连接失败
            cout<<"创建连接失败"<<endl;
            exit(1);
        }
        con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0);

        if(con == NULL){
            //连接失败
            cout<<mysql_error(con)<<endl;
            cout<<"连接失败"<<endl;
            exit(1);
        }

        connList.push_back(con);
        freeConn++;
    }

    //初始化信号量sem，其值代表着当前连接池中空闲的连接
    sem = My_sem(freeConn);
}

//从连接池中获取一个连接
MYSQL* connection_pool::getConnection(){
    MYSQL* con = NULL;

    if(connList.empty()) return NULL;

    //先对信号量进行P操作，实现同步访问
    sem.wait();

    //互斥访问访问连接池
    pool_mutex.lock();

    con = connList.front();
    connList.pop_front();

    freeConn--;
    usedConn++;
    
    pool_mutex.unlock();
    return con;
}

//释放当前连接
bool connection_pool::releaseConnection(MYSQL* con){
    if(con == NULL){
        return false;
    }
    
    pool_mutex.lock();
    connList.push_back(con);

    usedConn--;
    freeConn++;

    pool_mutex.unlock();
    return true;
}

//查看当前空闲连接数
int connection_pool::getFreeNum(){
    return freeConn;
}


//初始化RAII资源
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
    *SQL = connPool->getConnection();

    con = *SQL;
    pool = connPool;
}

//当对象析构时，释放连接
connectionRAII::~connectionRAII(){
    pool->releaseConnection(con);
}

