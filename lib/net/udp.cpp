#include "udp.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <map>
#include <thread>
#include <string>
#include "packet.h"
//#include "tcpiostream.h"

#define MSS 1024

int sock_fd;
struct sockaddr_in addr_serv;

char recv_pack[MSS+100];
std::map<std::string, struct sockaddr_in> sockaddr_map;

void (*udp_recv_callback)(const char* IP, const char* data, int size);

void client_init(const char* DEST_IP, uint16_t DEST_port){
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(DEST_port);
    addr_serv.sin_addr.s_addr = inet_addr(DEST_IP);

    sockaddr_map[std::string(inet_ntoa(addr_serv.sin_addr))] = addr_serv;
}

void server_init(const char* DEST_IP, uint16_t DEST_port){
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(DEST_port);
    addr_serv.sin_addr.s_addr = inet_addr(DEST_IP);

    if(bind(sock_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0){
        perror("bind error:");
        exit(1);
    }
}

void udp_recv(){
    int len = sizeof(addr_serv);

    while(true){
        int recv_num = recvfrom(sock_fd, recv_pack, MSS+100, 0, (struct sockaddr *)&addr_serv, (socklen_t *)&len);

        //printf("UDP receive sequence number: %d\n", reinterpret_cast<const segment*>(recv_pack)->header.seq);

        if (sockaddr_map.count(inet_ntoa(addr_serv.sin_addr)) == 0) {
            sockaddr_map[inet_ntoa(addr_serv.sin_addr)] = addr_serv;
        }
        if (recv_num < 0){
            perror("recvfrom error:");
            exit(1);
        }

        udp_recv_callback(inet_ntoa(addr_serv.sin_addr), recv_pack, recv_num);
    }

    close(sock_fd);
}

void udp_send(const char* IP, const char* data, int size){

    //printf("UDP send sequence number: %d\n", reinterpret_cast<const segment*>(data)->header.seq);

    int send_num = sendto(sock_fd, data, size, 0, (struct sockaddr *)&sockaddr_map[IP], sizeof(sockaddr_map[IP]));
    if (send_num < 0){
        perror("sendto error:");
        exit(1);
    }

}

void set_recv_callback(void (*callback)(const char* IP, const char* data, int size)) {
    udp_recv_callback = callback;
}
