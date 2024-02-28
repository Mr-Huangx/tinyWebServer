#ifndef HTTP_CONN_H
#define HTTP_CONN_H


#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>


#include"../utils/utils.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../lock/my_lock.h"


class http_conn{
public:
    static const int FILENAME_LEN = 200;    //最大文件名长度
    static const int READ_BUFFER_SIZE = 2048;//读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;//写缓冲区大小
    enum METHOD
    {
        //解析请求方法
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        //主状态机可能得状态：正在解析请求行，正在分析请求头，正在分析内容
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        //服务器处理HTTP请求的结果
        NO_REQUEST,//请求不完整，仍需要读取客户数据
        GET_REQUEST,//获取一个完整的请求
        BAD_REQUEST,//请求语法出错
        NO_RESOURCE,//资源没找到
        FORBIDDEN_REQUEST,//客户对于资源的权限不足
        FILE_REQUEST,//文件请求
        INTERNAL_ERROR,//服务器内部出错
        CLOSED_CONNECTION//客户端已经关闭连接
    };
    enum LINE_STATUS
    {
        //从状态机可能的状态：读取到完整的行（包括请求行和头部），行出错，行数据不完整
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *,int, int, string user, string passwd, string sqlname); 
    
    bool read_once();//proactor模式，一次性将所有数据进行处理

    bool write();//向客户端发送数据

    void process();//处理http请求

    void close_conn(bool real_close = true);//关闭一个socket连接

    void initmysql_result(connection_pool *connPool);//初始化该http请求的数据库读取表

private:
    void init();//初始化当前http连接中的各项参数，check_state默认为读请求行

    LINE_STATUS parse_line();//从状态机，用于分析出一行内容

    HTTP_CODE parse_requestline(char* text);//分析请求行

    HTTP_CODE parse_headers(char *text);//分析头部字段

    HTTP_CODE parse_request_content(char *text);//分析request中的content部分

    HTTP_CODE parse_content();//分析http请求的入口函数

    HTTP_CODE deal_request();//处理http请求

    HTTP_CODE process_read();

    bool process_write(HTTP_CODE ret);//

    bool add_response(const char *format, ...);//

    bool add_content(const char *content);

    bool add_status_line(int status, const char *title);

    bool add_headers(int content_length);

    bool add_content_type();

    bool add_content_length(int content_length);

    bool add_linger();

    bool add_blank_line();

    void unmap();//用于释放使用mmap分配的内存空间

public:
    static int epollfd;         //内核epoll_create返回值
    static int user_count;
    MYSQL* mysql;               //用于记录当前http请求由哪个连接池处理
    int state;                  //读为0，写为1
    static map<string, string> users;//记录数据库中所有用户的账户和密码
    static My_lock mutex;

private:
    int sockfd;                 //当前http请求使用的socket
    sockaddr_in address;
    char read_buf[READ_BUFFER_SIZE];    //读缓冲区
    long read_idx;                      //记录读的idx
    int checked_idx;
    int start_line;
    char write_buf[WRITE_BUFFER_SIZE];
    int write_idx;
    CHECK_STATE check_state;
    METHOD method;
    char real_file[FILENAME_LEN];

    //解析http请求中头部各字段的值
    char* url;
    char* version;
    char* host;
    long content_length;
    bool linger;
    char* file_address;

    struct stat file_stat;      //记录文件属性
    struct iovec iv[2];         //IO向量，提高磁盘到内存的效率
    int iv_count;
    int cgi;                    //是否处理post请求
    char *request_head;         //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char* source;               //服务器中可以请求到的资源根路径
    int TRIGMode;
    int close_log;

    int improv;                 //用于记录当前http请求是否处理完成
    // map<string, string> users;  //记录数据库中所有的用户和密码
    
    char sql_user[100];
    char sql_password[100];
    char sql_name[100];

    //工具箱
    Utils utils;
};


#endif