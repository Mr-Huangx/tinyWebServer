CXX ?= g++

server: main.cpp   webserver.cpp config.cpp ./http/http_conn.cpp ./CGImysql/sql_connection_pool.cpp  ./threadpool/threadpool.cpp ./utils/utils.cpp
	$(CXX) -o server  $^ -lpthread -lmysqlclient

clean:
	rm  -r server
