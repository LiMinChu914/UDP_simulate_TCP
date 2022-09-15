#include"packet.h"

void set_packetH(std::shared_ptr<segment>& pack,uint16_t source,uint16_t dest,uint32_t seq,uint32_t ack,uint16_t hl,uint16_t ACK,uint16_t SYN,uint16_t FIN,uint16_t rwnd,uint16_t checkSum){
    //std::unique_ptr<segment> pack(reinterpret_cast<segment*>(operator new(sizeof(segment))));
    pack->header.source = source;
    pack->header.dest = dest;
    pack->header.seq = seq;
    pack->header.ack = ack;
    pack->header.hl = hl;
    pack->header.ACK = ACK;
    pack->header.SYN = SYN;
    pack->header.FIN = FIN;
    pack->header.rwnd = rwnd;
    pack->header.checkSum = checkSum;
}