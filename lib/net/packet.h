#ifndef __PACKET_H__
#define __PACKET_H__

#include <iostream>
#include <memory>

typedef struct{
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack;
    uint16_t hl : 4;
    uint16_t zero : 6;
    uint16_t URG : 1;
    uint16_t ACK : 1;
    uint16_t PSH : 1;
    uint16_t RST : 1;
    uint16_t SYN : 1;
    uint16_t FIN : 1;
    uint16_t rwnd;
    uint16_t checkSum;
    uint16_t URG_p;
} Header;

struct segment {
    Header header;
    uint8_t data[];
};

void set_packetH(std::shared_ptr<segment>& pack, uint16_t source = 0,uint16_t dest = 0,uint32_t seq = 0,uint32_t ack = 0,uint16_t hl = 0,uint16_t ACK = 0,uint16_t SYN = 0,uint16_t FIN = 0,uint16_t rwnd = 0,uint16_t checkSum = 0);

#endif