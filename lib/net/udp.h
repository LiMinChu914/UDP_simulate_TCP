#ifndef __UDP_H__
#define __UDP_H__
#include <cstdint>


void client_init(const char* DEST_IP, uint16_t DEST_port);

void server_init(const char* DEST_IP, uint16_t DEST_port);

void udp_recv();
void udp_send(const char* IP, const char* data, int size);

void set_recv_callback(void (*callback)(const char* IP, const char* data, int size));

#endif