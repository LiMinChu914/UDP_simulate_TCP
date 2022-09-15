#ifndef __TCP_DUPLEX_H__
#define __TCP_DUPLEX_H__

#include<memory>
#include"packet.h"


void tcp_duplex(std::shared_ptr<segment> pack, int size);


#endif