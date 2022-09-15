#ifndef __SENDER_H__
#define __SENDER_H__

#include<cstdint>
#include<memory>
#include"tcpiostream.h"

void sender(std::shared_ptr<tcp_link_info> table, const char* in, int size);

bool send_new(std::shared_ptr<tcp_link_info> table);

bool fast_retransmit(std::shared_ptr<tcp_link_info> table);

#endif