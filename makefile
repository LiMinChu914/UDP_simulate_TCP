CC = g++

all: Client Server

Client: clean Client.o udp.o tcpiostream.o packet.o async.o section_control.o global_var.o tcp.o receiver.o buffer.o tcp_duplex.o sender.o EventDispatcher.o
	$(CC) -pthread -o $@ Client.o udp.o tcpiostream.o packet.o async.o section_control.o global_var.o tcp.o receiver.o buffer.o tcp_duplex.o sender.o EventDispatcher.o

Server: clean Server.o udp.o tcpiostream.o packet.o async.o section_control.o global_var.o tcp.o receiver.o buffer.o tcp_duplex.o sender.o EventDispatcher.o
	$(CC) -pthread -o $@ Server.o udp.o tcpiostream.o packet.o async.o section_control.o global_var.o tcp.o receiver.o buffer.o tcp_duplex.o sender.o EventDispatcher.o

%.o: */*/%.cpp
	$(CC) -pthread -c $<

.PHONY: clean
clean:
	@echo "Clean..."
	-rm *.o Server Client
