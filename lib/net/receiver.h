#ifndef __RECEIVER_H__
#define __RECEIVER_H__


#include <cstdint>
#include "packet.h"


void receiver(std::shared_ptr<segment> pack, int size, bool not_send_ack);

#endif