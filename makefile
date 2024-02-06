CXX ?= g++

server: main.cpp   webserver.cpp config.cpp ./http/http_conn.cpp ./CGImysql/sql_connection_pool.cpp ./lock/my_cond.h ./loc/my_lock.h ./loc/my_sem.h ./threadpool/threadpool.cpp ./utils/utils.cpp
	$(CXX) -o server  $^ 

clean:
	rm  -r server
