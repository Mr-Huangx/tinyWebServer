CXX ?= g++

server: main.cpp   webserver.cpp config.cpp ./http/http_conn.cpp ./CGImysql/sql_connection_pool.cpp
	$(CXX) -o server  $^ 

clean:
	rm  -r server
