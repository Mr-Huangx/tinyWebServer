#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unistd.h>
#include<string>
#include<stdlib.h>

class WebServer{
public:
    void init(string user, string passwd, string database,
            int PORT, int LOGWrite, int OPT_LINGER, int TRIGMode,
            int sql_num, int thread_num,int close_log, int actor_model);



};


#endif
