#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string>
#include<stdlib.h>

#include"../lock/my_lock.h"
#include"../lock/my_sem.h"
using namespace std;

class connection_pool{
public:
    static connection_pool* getInstance();//单例模式

    void init(string url, string user, string password, string database, int port, int sql_num);//初始化数据库各项参数

    MYSQL* getConnection();      //获取数据库连接

    bool releaseConnection(MYSQL* conn);    //释放连接

    int getFreeNum();//当前空闲连接数
    

private:
    connection_pool();
    ~connection_pool();

    int maxConn;    //连接池最大连接数
    int usedConn;   //当前已使用的连接数
    int freeConn;   //当前空闲的连接数
    My_lock pool_mutex;  //互斥访问连接池
    list<MYSQL*> connList; //连接池
    My_sem sem;     //信号量


    string url;     //主机url
    string port;    //数据库端口号
    string user;    //登录数据库的用户名
    string password;//登录数据库的密码
    string databasename;//使用的数据库名
    
};

//上面完成了连接池的设计
//接下来将其封装成RAII资源，以减少编写中手动释放资源
class connectionRAII{
public:
    connectionRAII(MYSQL **SQL, connection_pool *connPool);
    ~connectionRAII();

private:
    connection_pool* pool;
    MYSQL *con;
};


#endif