#include <future>
#include <memory>
#include <random>
#include <string.h>
#include "tcpiostream.h"
#include "../tool/async.h"
#include "packet.h"
#include "../../global_var.h"
#include "../tool/buffer.h"
#include "tcp.h"
#include "tcp_duplex.h"

std::random_device d_rd;
std::default_random_engine d_generator( d_rd() );
std::uniform_real_distribution<float> d_unif(0.0, 1.0);

std::map<struct key, std::shared_ptr<tcp_link_info> > tcp_port_table;

void (*tcp_ostream)(const char* IP, const char* data, int size, double loss_rate);

uint64_t timer = -1, deadline = -1;

void tcp_istream(const char* IP, const char* data, int size){
    //std::cout << "Enter \"void tcp_istream(const char* IP, const char* data, int size)\"\n";
    std::shared_ptr<segment> pack((segment*)operator new(size));
    memcpy(pack.get(), data, size);

    bool leave = false;

    setTimeout(
        [=, &leave](){
            //printf("receive pack size %d\n", size);

            key k;
            k.s = pack->header.dest;
            k.d = pack->header.source;
            //3-way hand-shake
            if (pack->header.SYN == 1 && pack->header.ACK == 1){
                //printf("abcde\n");
                if (tcp_port_table.count(k) != 0 && tcp_port_table[k]->state == 0){
                    auto& table = tcp_port_table[k];
                    table->Dest_IP = IP;
                    table->cwnd = MSS;
                    table->nextSeqNum = table->sendBase = table->sendEnd = pack->header.ack;
                    table->lastByteRead = pack->header.seq+1;
                    table->expectSeqNum = pack->header.seq+1;
                    table->rwnd = RWND_MAX;
                    table->state = 1;
                    table->recv_tool->clear(pack->header.seq+1);
                    table->send_tool->clear(pack->header.ack);

                    std::shared_ptr<segment> send_pack((segment*)operator new(sizeof(segment)));
                    set_packetH(send_pack, pack->header.dest, pack->header.source, pack->header.ack, table->expectSeqNum, 5, 1, 0, 0, RWND_MAX, 0);

                    tcp_ostream(IP, reinterpret_cast<const char*>(&*send_pack), 20, LOSS_RATE);
                    printf("\tReceive a packet (SYN/ACK) with seq_num = %d, ack_num = %d from %s : %d\n\tSend a packet (ACK) with ack_num = %d and seq_num = %d to %s : %d\n====3-way hand-shape end====\n", pack->header.seq, pack->header.ack, IP, pack->header.source, send_pack->header.ack, send_pack->header.seq, IP, pack->header.source);
                    tcp_port_table[k]->get_SYN_ACK = 1;
                }
                //printf("syn ack\n");
                leave = true;
                return;
            }
            else if (pack->header.SYN == 1 && pack->header.ACK == 0){
                if (tcp_port_table.count(k) != 0 || getSYN){
                    leave = true;
                    return;
                }
                printf("Receive a SYN packet, Server accept!\n\treceive a SYN with sequence number %d\n", pack->header.seq);
                int initSeqNum = (d_unif(d_generator) * 9999) + 1;
                //int initSeqNum = 0;

                std::shared_ptr<segment> syn_ack((segment*)operator new(sizeof(segment)));
                set_packetH(syn_ack, pack->header.dest, pack->header.source, initSeqNum, pack->header.seq+1, 5, 1, 1, 0, RWND_MAX, 0);

                tcp_ostream(IP, reinterpret_cast<const char*>(&*syn_ack), 20, LOSS_RATE);
                timer = setInterval(
                    [&, IP](){
                        tcp_ostream(IP, reinterpret_cast<const char*>(&*syn_ack), 20, LOSS_RATE);
                    },
                    500
                );
                deadline = setTimeout(
                    [=](){
                        clear_event(timer);
                        syn_ack_failed = true;
                    },
                    5000
                );
                printf("Send SYN/ACK with seq %d, ack %d\n", syn_ack->header.seq, syn_ack->header.ack);

                std::shared_ptr<tcp_link_info> info = std::make_shared<tcp_link_info>();
                info->Dest_IP = IP;
                info->Dest_port = pack->header.source;
                info->Source_port = pack->header.dest;
                info->state = 0;
                info->delayAckTimer = -1;
                info->duplicate_count = 0;
                info->senderTimer = -1;
                info->rwnd = RWND_MAX;
                info->ssthresh = THRESHOLD;
                info->send_tool = std::make_shared<buffer_tool>(info->send_buf, SEND_BUF_SIZE);
                info->recv_tool = std::make_shared<buffer_tool>(info->recv_buf, RECV_Buf_SIZE);
                tcp_port_table[k] = info;

                client_port = pack->header.source;
                getSYN = true;
                leave = true;

                //info->enable_debugger();
                return;
            }

            //deliver segment to correct port
            if (tcp_port_table.count(k) == 0){
                leave = true;
                return;
            }


            if (pack->header.ACK == 1 && !get3wayACK && tcp_port_table[k]->state == 0){
                auto &table = tcp_port_table[k];
                clear_event(timer);
                clear_event(deadline);

                printf("Receive ACK with seq %d, ack %d\n", pack->header.seq, pack->header.ack);
                table->state = 1;
                table->cwnd = MSS;
                table->sendBase = table->sendEnd = table->nextSeqNum = pack->header.ack;
                table->lastByteRead = pack->header.seq;
                table->expectSeqNum = pack->header.seq;
                table->recv_tool->clear(pack->header.seq);
                table->send_tool->clear(pack->header.ack);
                key_table[table->Dest_port] = key{table->Source_port, table->Dest_port};
                get3wayACK = true;
            }
            
            tcp_duplex(pack, size);
            leave = true;
        },
        0
    );

    await(
        [&](){
            //std:: cout << "======================================= leave: " << leave << std::endl; 
            return leave;
        }
    );
    //std::cout << "Leave \"void tcp_istream(const char* IP, const char* data, int size)\"\n";

}

void set_tcp_ostream(void (*work)(const char* IP, const char* data, int size, double loss_rate)){
    tcp_ostream = work;
}
