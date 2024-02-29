#include "http_conn.h"

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

int http_conn::user_count = 0;
int http_conn::epollfd = -1;//epoll_create返回的结果

map<string, string> http_conn::users;
My_lock http_conn::mutex;

void http_conn::init(int m_sockfd, const sockaddr_in &m_addr, char *m_source,int m_TRIGMode,
                    int m_close_log,string m_user, string m_passwd, string m_sqlname)
{
    //只有构造函数能使用列表初始化
    sockfd = m_sockfd;
    address = m_addr;
    source = m_source;
    utils.addfd(epollfd, sockfd, true, m_TRIGMode);
    user_count++;

    //当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
    TRIGMode = m_TRIGMode;
    close_log = m_close_log;

    strcpy(sql_user, m_user.c_str());
    strcpy(sql_password, m_passwd.c_str());
    strcpy(sql_name, m_sqlname.c_str());

    init();
}

//初始化新接受的连接
//check_state默认为分析请求行状态
void http_conn::init(){
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    check_state = CHECK_STATE_REQUESTLINE;
    linger = false;
    method = GET;
    url = 0;
    version = 0;
    content_length = 0;
    host = 0;
    start_line = 0;
    checked_idx = 0;
    read_idx = 0;
    write_idx = 0;
    cgi = 0;
    state = 0;
    

    memset(read_buf, '\0', READ_BUFFER_SIZE);
    memset(write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(real_file, '\0', FILENAME_LEN);
}

//以下部分如若有疑问，请参考《Linux高性能服务器编程》--游双，P137
//从状态机，用于分析出一行内容(包括请求行和请求头部)
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    //read_idx表示客户端数据的尾部下一个字节,checked_idx表示正在解析的字节
    for (; checked_idx < read_idx; ++checked_idx)
    {
        //获取当前要分析的字节
        temp = read_buf[checked_idx];
        //如果为"\r"，即回车符，说明可能读取到了一个完整的行
        if (temp == '\r')
        {
            //如果"\r"字符碰巧是buffer中的最后一个已经被读入的客户数据，那么这次分析没有读取
            //到一个完整的行，返回LINE_OPEN以表示还需要继续读取客户数据才能进一步分析
            if ((checked_idx + 1) == read_idx)
                return LINE_OPEN;

            //如果是"\n"，则说明我们成功读取到一个完整的行
            else if (read_buf[checked_idx + 1] == '\n')
            {
                read_buf[checked_idx++] = '\0';
                read_buf[checked_idx++] = '\0';
                return LINE_OK;
            }
            //否则，http请求语法存在问题
            return LINE_BAD;
        }

        //如果当前字节为"\n"，即换行符，可能是读取到了一个完整的行
        else if (temp == '\n')
        {
            if (checked_idx > 1 && read_buf[checked_idx - 1] == '\r')
            {
                read_buf[checked_idx - 1] = '\0';
                read_buf[checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    //如果所有内容都分析完毕，也没有遇到"\r"字符，则返回LINE_OPEN,表示还需要继续读取客户数据才能进一步分析
    return LINE_OPEN;
}

//分析请求行
http_conn::HTTP_CODE http_conn::parse_requestline(char* text){
    url = strpbrk(text, " \t");//返回在temp中首次出现"空格"/"\t"字符的位置指针

    //如果请求行中没有空白字符或者"\t"字符，则HTTP请求必然有问题
    if( !url ) return BAD_REQUEST;


    *url++ = '\0';//将出现 空白、\t字符的位置设置为字符串结尾，因此，text就只保留了method字符串
    char* method = text;
    
    if( strcasecmp(method, "GET") == 0){
        //如果是get方法
        this->method = GET;
    }
    else if(strcasecmp(method, "POST") == 0){

        //如果是post方法
        this->method = POST;
        cgi = 1;
    }
    else{
        //只支持GET、POST方法，其他都返回错误
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");////返回url中首个不与"空格"/"空制表符"相同的字符的位置
    version = strpbrk(url, " \t");
    if( !version ){
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");

    //仅支持http/1.1，其他的不支持
    if (strcasecmp(version, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    //支持http和https协议
    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');//搜索m_url字符串中，第一次出现'/'的位置，并返回指针指向字符串中的字符，没有发现则为NULL
    }

    if (strncasecmp(url, "https://", 8) == 0)//与上一个相同，只是区别于http请求还是https请求
    {
        url += 8;
        url = strchr(url, '/');
    }

    if (!url || url[0] != '/')
        return BAD_REQUEST;

    //当url为/时，显示判断界面
    if (strlen(url) == 1)
        strcat(url, "judge.html");

    //http请求行处理完毕,状态转移到对头部字段的分析
    check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//分析头部字段
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
    //遇到一个空行，说明我们得到了一个正确的HTTP请求
    if (text[0] == '\0'){
        if (content_length != 0)
        {
            check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }

        return GET_REQUEST;
    }
    else if(strncasecmp(text, "Host:", 5) == 0){
        //处理Host请求头部
        text += 5;
        text += strspn(text, "\t");
        host = text;
    }
    else if(strncasecmp(text, "Connection:", 11) == 0){
        //处理connection请求头部
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            // linger = true;
        }
    }
    else if(strncasecmp(text, "Content-length:", 15) == 0){
        //处理content长度字段
        text += 15;
        text += strspn(text, " \t");
        content_length = atol(text);
    }
    else{
        //只处理Host/connection/content_length字段，其他不处理

    }
    return NO_REQUEST;
}
//分析http请求中的content部分
http_conn::HTTP_CODE http_conn::parse_request_content(char *text){
    if (read_idx >= (content_length + checked_idx))
    {
        text[content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        request_head = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

//分析http请求的入口函数
http_conn::HTTP_CODE http_conn::parse_content()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while((check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || (line_status = parse_line()) == LINE_OK)
    {
        text = read_buf + start_line;
        start_line = checked_idx;

        switch (check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                //如果是正在解析请求行
                ret = parse_requestline(text);
                if(ret == BAD_REQUEST) return BAD_REQUEST;
                break;
            }
            

            case CHECK_STATE_HEADER:
            {
                //解析请求头部
                ret = parse_headers(text);
                if(ret == BAD_REQUEST) return BAD_REQUEST;
                else if(ret == GET_REQUEST){
                    //获取到了http请求
                    return deal_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = parse_request_content(text);
                if (ret == GET_REQUEST)
                    return deal_request();
                line_status = LINE_OPEN;
                break;
            }
            
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

//处理http请求
http_conn::HTTP_CODE http_conn::deal_request()
{
    strcpy(real_file, source);
    int len = strlen(source);

    //strrchr()返回url中最后一次出现/的位置指针;strchr()返回第一次出现的位置指针
    const char* p = strrchr(url, '/');

    //处理cgi
    if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        //根据标志判断是登录检测还是注册检测
        char flag = url[1];

        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/");
        strcat(url_real, url + 2);
        strncpy(real_file + len, url_real, FILENAME_LEN - len - 1);
        free(url_real);

        //获取用户名和密码,这里需要后面来看看http请求的结构
        char name[100], password[100];
        int i;
        for (i = 5; request_head[i] != '&'; ++i)
            name[i - 5] = request_head[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; request_head[i] != '\0'; ++i, ++j)
            password[j] = request_head[i];
        password[j] = '\0';

        if (*(p + 1) == '3')
        {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (users.find(name) == users.end())
            {
                mutex.lock();
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                mutex.unlock();

                if (!res)
                    strcpy(url, "/log.html");
                else
                    strcpy(url, "/registerError.html");
            }
            else
                strcpy(url, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if (*(p + 1) == '2')
        {
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(url, "/welcome.html");
            else
                strcpy(url, "/logError.html");
        }  
    }

    if (*(p + 1) == '0')
    {
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/register.html");
        strncpy(real_file + len, url_real, strlen(url_real));

        free(url_real);
    }
    else if (*(p + 1) == '1')
    {
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/log.html");
        strncpy(real_file + len, url_real, strlen(url_real));

        free(url_real);
    }
    else if (*(p + 1) == '5')
    {
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/picture.html");
        strncpy(real_file + len, url_real, strlen(url_real));

        free(url_real);
    }
    else if (*(p + 1) == '6')
    {
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/video.html");
        strncpy(real_file + len, url_real, strlen(url_real));

        free(url_real);
    }
    else if (*(p + 1) == '7')
    {
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/fans.html");
        strncpy(real_file + len, url_real, strlen(url_real));

        free(url_real);
    }
    else
        strncpy(real_file + len, url, FILENAME_LEN - len - 1);

    if (stat(real_file, &file_stat) < 0)
        return NO_RESOURCE;

    if (!(file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(file_stat.st_mode))
        return BAD_REQUEST;

    int fd = open(real_file, O_RDONLY);
    //申请一段内存空间，这段内存可以作为进程间通信的共享内存
    file_address = (char *)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap()
{
    //释放由mmap申请的内存空间
    if(file_address){
        //如果存在内存空间则释放
        munmap(file_address, file_stat.st_size);
    }
}

//proactor模式，一次性将所有数据进行处理
bool http_conn::read_once()
{
    if (read_idx >= READ_BUFFER_SIZE)
    {
        printf("read_idx >= read_buffer_size\n");
        return false;
    }
    int bytes_read = 0;

    //LT读取数据
    if (0 == TRIGMode)
    {   
        bytes_read = recv(sockfd, read_buf + read_idx, READ_BUFFER_SIZE - read_idx, 0);
        read_idx += bytes_read;

        if (bytes_read <= 0)
        {
            printf("bytes_read <= 0\n");
            return false;
        }
        // printf("进行数据读取完成,sock传输数据为:%s\n", read_buf + read_idx);
        return true;
    }
    //ET读数据
    else
    {
        while (true)
        {
            bytes_read = recv(sockfd, read_buf + read_idx, READ_BUFFER_SIZE - read_idx, 0);
            if (bytes_read == -1)
            {
                printf("ET read failed\n");
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0)
            {
                printf("ET bytes_read == 0\n");
                return false;
            }
            read_idx += bytes_read;
        }
        return true;
    }
}

bool http_conn::write()
{
    //向客户端发送数据
    // printf("向客户端socket:%d 发送数据\n", sockfd);
    int temp = 0;

    if (bytes_to_send == 0)
    {
        utils.modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
        init();
        return true;
    }

    while (1)
    {
        temp = writev(sockfd, iv, iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                // printf("writev调用返回值-0， socket:%d 重新被放入epollfd中\n", sockfd);
                utils.modfd(epollfd, sockfd, EPOLLOUT, TRIGMode);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= iv[0].iov_len)
        {
            iv[0].iov_len = 0;
            iv[1].iov_base = file_address + (bytes_have_send - write_idx);
            iv[1].iov_len = bytes_to_send;
        }
        else
        {
            iv[0].iov_base = write_buf + bytes_have_send;
            iv[0].iov_len = iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0)
        {
            unmap();
            // printf("发送成功socket:%d 重新被放入epollfd中\n", sockfd);
            
            if(linger) utils.modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
            else{
                //关闭连接
                
            }

            if (linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

void http_conn::process()
{
    // cout<<"processing...\n";
    HTTP_CODE read_ret = parse_content();
    if (read_ret == NO_REQUEST)
    {
        printf("当前任务没有http_request，对socket \"%d\"进行重新处理\n", sockfd);
        utils.modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        //write出错，直接关闭当前连接
        printf("处理当前任务的时候，write出错，关闭socket:%d\n", sockfd);
        close_conn();
    }
    // printf("处理完当前任务，对socket \"%d\", 重新放入epollfd中\n", sockfd);
    utils.modfd(epollfd, sockfd, EPOLLOUT, TRIGMode);
}

http_conn::HTTP_CODE http_conn::process_read()
{
    //process_read 和parsecontent一致

    // LINE_STATUS line_status = LINE_OK;
    // HTTP_CODE ret = NO_REQUEST;
    // char *text = 0;

    // while ((check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    // {
    //     text = get_line();
    //     start_line = checked_idx;
    //     LOG_INFO("%s", text);
    //     switch (check_state)
    //     {
    //     case CHECK_STATE_REQUESTLINE:
    //     {
    //         ret = parse_request_line(text);
    //         if (ret == BAD_REQUEST)
    //             return BAD_REQUEST;
    //         break;
    //     }
    //     case CHECK_STATE_HEADER:
    //     {
    //         ret = parse_headers(text);
    //         if (ret == BAD_REQUEST)
    //             return BAD_REQUEST;
    //         else if (ret == GET_REQUEST)
    //         {
    //             return do_request();
    //         }
    //         break;
    //     }
    //     case CHECK_STATE_CONTENT:
    //     {
    //         ret = parse_content(text);
    //         if (ret == GET_REQUEST)
    //             return do_request();
    //         line_status = LINE_OPEN;
    //         break;
    //     }
    //     default:
    //         return INTERNAL_ERROR;
    //     }
    // }
    // return NO_REQUEST;
}

//关闭连接，关闭一个连接，客户总量减一
void http_conn::close_conn(bool real_close)
{
    if (real_close && (sockfd != -1))
    {
        printf("close %d\n", sockfd);
        utils.removefd(epollfd, sockfd);
        sockfd = -1;
        user_count--;
    }
}

//初始化http连接的读取表
void http_conn::initmysql_result(connection_pool *connPool)
{
    //先从连接池中取一个连接
    MYSQL *mysql = NULL;
    connectionRAII mysqlcon(&mysql, connPool);

    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        // LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
        cout<<"select出错"<<endl;
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

bool http_conn::add_response(const char *format, ...)
{
    if (write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);//获取format之后的变参
    int len = vsnprintf(write_buf + write_idx, WRITE_BUFFER_SIZE - 1 - write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - write_idx))
    {
        va_end(arg_list);
        return false;
    }
    write_idx += len;
    va_end(arg_list);

    // LOG_INFO("request:%s", write_buf);

    return true;
}

//添加状态行
bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

//添加响应头部
bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}

//添加响应头部中的内容长度
bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}

//添加响应头部的connection字段
bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n", (linger == true) ? "keep-alive" : "close");
}

//添加响应头部的内容类别
bool http_conn::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}

//添加响应头部空行
bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

//添加内容
bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (file_stat.st_size != 0)
        {
            add_headers(file_stat.st_size);
            iv[0].iov_base = write_buf;
            iv[0].iov_len = write_idx;
            iv[1].iov_base = file_address;
            iv[1].iov_len = file_stat.st_size;
            iv_count = 2;
            bytes_to_send = write_idx + file_stat.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    iv[0].iov_base = write_buf;
    iv[0].iov_len = write_idx;
    iv_count = 1;
    bytes_to_send = write_idx;
    return true;
}