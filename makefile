CXX ?= g++ 
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 

SRCS = main.cpp \
 		./http/http_conn.cpp \
  		./CGImysql/sql_connection_pool.cpp \
   		./threadpool/threadpool.cpp \
    	./utils/utils.cpp \
	 	webserver.cpp \
	 	config.cpp \
		./log/log.cpp \
		./timer/lst_timer.cpp
OBJS = $(SRCS:.cpp=.o) 
server: $(OBJS) 
	$(CXX) -o $@ $^ -g -lpthread -L/usr/lib64/mysql -lmysqlclient 
%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $< 

clean:
	rm -f $(OBJS) server
