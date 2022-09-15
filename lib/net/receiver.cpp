#include "receiver.h"
#include "tcpiostream.h"
#include "../tool/section_control.h"
#include "../tool/async.h"
#include "../tool/buffer.h"
#include <algorithm>

void receiver(std::shared_ptr<segment> pack, int size, bool not_send_ack){
    //put in buffer
    //move slide
    //rwnd

    key k;
    k.s = pack->header.dest;
    k.d = pack->header.source;
    auto& table = tcp_port_table[k];

    int data_length = size - pack->header.hl*4;
    if(data_length == 0)
        return;
    char* data_ptr = reinterpret_cast<char*>(pack.get()) + (pack->header.hl * 4);
    //printf("packet content: size %d\n", data_length);
    /*for(int i = 0; i < data_length; i++){
        printf("%c", data_ptr[i]);
    }
    printf("]\n");*/
    //printf("seq: %d, expect: %d, rwnd: %d, start: %d, end: %d\n", pack->header.seq, table->expectSeqNum, table->rwnd, table->recv_tool->start, table->recv_tool->end);

    //printf("[%d, %d]\n[%d, %d]\n", pack->header.seq, pack->header.seq+size-pack->header.hl*4, table->expectSeqNum, table->expectSeqNum+table->rwnd);

    if(pack->header.seq + data_length > table->expectSeqNum && pack->header.seq < table->expectSeqNum+table->rwnd){
        int start_seq = pack->header.seq;
        if(pack->header.seq < table->expectSeqNum){
            data_ptr += (table->expectSeqNum-pack->header.seq);
            data_length -= (table->expectSeqNum-pack->header.seq);
            start_seq = table->expectSeqNum;
        }
        if(start_seq + data_length > table->expectSeqNum+table->rwnd){
            data_length = table->expectSeqNum + table->rwnd - start_seq;
        }

        //printf("[%d, %d]\n", start_seq, start_seq+data_length);
        //receiver action for ack
        if(start_seq == table->expectSeqNum && table->recvControl.empty() && table->delayAckTimer == -1){
            int k = table->recv_tool->write(data_ptr, data_length, start_seq);
            //printf("\n    write: %d\n\n", k);
            table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());
            table->expectSeqNum += data_length;
            table->delayAckTimer = setTimeout(
                [=](){
                    //send ACK
                    if(not_send_ack)
                        return;
                    std::shared_ptr<segment> ack_pack((segment*)operator new(sizeof(segment)));
                    set_packetH(ack_pack, pack->header.dest, pack->header.source, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, std::min(RWND_MAX, table->recv_tool->remain()), 0);
                    tcp_ostream(table->Dest_IP.c_str(),reinterpret_cast<const char*>(&*ack_pack), 20, LOSS_RATE);
                    if(!print_loss)
                        printf("1. send ACK with ack: %d, seq: %d\n", ack_pack->header.ack, ack_pack->header.seq);
                },
                1000
            );
        }
        else if(start_seq == table->expectSeqNum && table->delayAckTimer != -1){
            table->recv_tool->write(data_ptr, data_length, start_seq);
            table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());
            table->expectSeqNum += data_length;

            clear_event(table->delayAckTimer);
            table->delayAckTimer = -1;

            //send ACK
            if(not_send_ack)
                return;
            std::shared_ptr<segment> ack_pack((segment*)operator new(sizeof(segment)));
            set_packetH(ack_pack, pack->header.dest, pack->header.source, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, std::min(RWND_MAX, table->recv_tool->remain()), 0);
            tcp_ostream(table->Dest_IP.c_str(),reinterpret_cast<const char*>(&*ack_pack), 20, LOSS_RATE);
            if(!print_loss)
                printf("2. send ACK with ack: %d, seq: %d\n", ack_pack->header.ack, ack_pack->header.seq);
        }
        else if(start_seq > table->expectSeqNum){
            table->recv_tool->write(data_ptr, data_length, start_seq);
            table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());

            clear_event(table->delayAckTimer);
            table->delayAckTimer = -1;
            table->recvControl.addSection({start_seq, start_seq+data_length});
            //send ACK
            if(not_send_ack)
                return;
            std::shared_ptr<segment> ack_pack((segment*)operator new(sizeof(segment)));
            set_packetH(ack_pack, pack->header.dest, pack->header.source, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, std::min(RWND_MAX, table->recv_tool->remain()), 0);
            tcp_ostream(table->Dest_IP.c_str(),reinterpret_cast<const char*>(&*ack_pack), 20, LOSS_RATE);
            printf("3. send LOSS ACK with ack: %d, seq: %d\n", ack_pack->header.ack, ack_pack->header.seq);
        }
        else{
            table->recv_tool->write(data_ptr, data_length, start_seq);
            table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());

            clear_event(table->delayAckTimer);
            table->delayAckTimer = -1;
            table->recvControl.addSection({start_seq, start_seq+data_length});
            table->expectSeqNum = table->recvControl.moveFirstSection();
            //send ACK
            if(not_send_ack)
                return;
            std::shared_ptr<segment> ack_pack((segment*)operator new(sizeof(segment)));
            set_packetH(ack_pack, pack->header.dest, pack->header.source, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, std::min(RWND_MAX, table->recv_tool->remain()), 0);
            tcp_ostream(table->Dest_IP.c_str(),reinterpret_cast<const char*>(&*ack_pack), 20, LOSS_RATE);
            if(!print_loss)
                printf("4. send ACK with ack: %d, seq: %d\n", ack_pack->header.ack, ack_pack->header.seq);
        }
        //printf("recv remain: %d\n", table->recv_tool->remain());
    }
    else{
        //send ACK
        if(not_send_ack)
            return;
        std::shared_ptr<segment> ack_pack((segment*)operator new(sizeof(segment)));
        set_packetH(ack_pack, pack->header.dest, pack->header.source, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, std::min(RWND_MAX, table->recv_tool->remain()), 0);
        tcp_ostream(table->Dest_IP.c_str(),reinterpret_cast<const char*>(&*ack_pack), 20, LOSS_RATE);
        printf("5. send LOSS ACK with ack: %d, seq: %d\n", ack_pack->header.ack, ack_pack->header.seq);
    }
}

