#ifndef __TCP_H__
#define __TCP_H__

#include<cstdint>
#include<map>
#include"tcpiostream.h"

extern std::map<uint16_t, key> key_table;

uint16_t connect(const char* Server_IP, uint16_t Server_port);
uint16_t accept(const char* Server_IP, uint16_t Server_port);


int read_till_ln(uint16_t port, char* buf);
int recv(uint16_t port, char* buf, int size);
void send(uint16_t port, const char* buf, int size);

void send_poisson(const char* IP, const char* data, int size, double loss_rate = 0);

#endif