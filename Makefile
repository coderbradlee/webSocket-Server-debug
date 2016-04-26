all:websocket

CXXFLAGS ?= -std=c++11 -g -O3 -L. -L/usr/local/lib -L. -lcrypto -lboost_program_options -lboost_filesystem -lboost_coroutine -lboost_system -lboost_thread -lpthread -lboost_context -lboost_date_time -lboost_log_setup -lboost_log -lssl

daemon:websocketDaemon

websocketDaemon: websocket.cpp base64.cpp modp_base64/modp_b64.cc renesolalog.cpp
	g++ $(CXXFLAGS) -DDAEMON modp_base64/modp_b64.cc renesolalog.cpp base64.cpp websocket.cpp -o websocketDaemon
	rm -f *.o

websocket: websocket.cpp base64.cpp modp_base64/modp_b64.cc renesolalog.cpp
	g++ $(CXXFLAGS) modp_base64/modp_b64.cc base64.cpp renesolalog.cpp websocket.cpp -o websocket
	rm -f *.o

clean:
	rm -f *.o
cleanall:
	rm -f *.o websocket websocketDaemon
