CXX ?= g++

server: main.cpp   webserver.cpp config.cpp
	$(CXX) -o server  $^ 

clean:
	rm  -r server
