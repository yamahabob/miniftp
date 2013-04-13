## makefile
all: client server

server: server.o data_link.o event_queue.o utilities.o
	g++ server.o data_link.o event_queue.o utilities.o -o server

client: client.o data_link.o event_queue.o utilities.o
	g++ client.o data_link.o event_queue.o utilities.o -o client

client.o: client.cpp client.h header.h
	g++ -c client.cpp
 
server.o: server.cpp server.h header.h
	g++ -c server.cpp

event_queue.o: event_queue.cpp event_queue.h
	g++ -c event_queue.cpp

utilities.o: utilities.cpp utilities.h
	g++ -c utilities.cpp

data_link.o: data_link.cpp data_link.h
	g++ -c data_link.cpp

clean: 
	rm -rf *.o client server
