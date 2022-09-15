#include"tcp.h"
#include <thread>
#include <fstream>
#include <string.h>
#include <random>
#include "udp.h"
#include "packet.h"
#include "../tool/async.h"
#include "../../global_var.h"
#include "../tool/buffer.h"
#include "sender.h"
#include <memory>


#define SERV_PORT 8000
#define SERV_IP "127.0.0.1"

std::random_device m_rd;
std::mt19937 m_generator( m_rd() );
std::uniform_real_distribution<float> m_unif(0.0, 1.0);

std::random_device l_rd;
std::mt19937 l_generator( l_rd() );
std::normal_distribution<double> normal(0.0, 1.0);


std::map<uint16_t, key> key_table;

uint16_t connect(const char* Server_IP, uint16_t Server_port){
    int initSeqNum = (m_unif(m_generator) * 9999) + 1;
    uint16_t PORT;
    while(PORT = m_unif(m_generator) * 65535, key_table.count(PORT) != 0);

    uint64_t timer, deadline;
    bool failed = false;
    key k;
    k.s = PORT;
    k.d = Server_port;

    bool l = false;

    setTimeout(
        [&](){
            std::shared_ptr<segment> pack((segment*)operator new(sizeof(segment)));
            set_packetH(pack, PORT, Server_port, initSeqNum, 0, 5, 0, 1, 0, RWND_MAX, 0);

            printf("====3-way hand-shake start====\n\tSend a packet(SYN) with initial seq_num %d to %s : %d\n", pack->header.seq, Server_IP, Server_port);   
            std::shared_ptr<tcp_link_info> info = std::make_shared<tcp_link_info>();
            info->get_SYN_ACK = 0;
            info->state = 0;
            info->Dest_IP = Server_IP;
            info->Dest_port = Server_port;
            info->Source_port = PORT;
            info->delayAckTimer = -1;
            info->senderTimer = -1;
            info->send_tool = std::make_shared<buffer_tool>(info->send_buf, SEND_BUF_SIZE);
            info->recv_tool = std::make_shared<buffer_tool>(info->recv_buf, RECV_Buf_SIZE);
            tcp_port_table[k] = info;

            //info->enable_debugger();

            tcp_ostream(tcp_port_table[k]->Dest_IP.c_str(), reinterpret_cast<const char*>(&*pack), 20, LOSS_RATE);

            timer = setInterval(
                [&, pack, k](){
                    tcp_ostream(tcp_port_table[k]->Dest_IP.c_str(), reinterpret_cast<const char*>(&*pack), 20, LOSS_RATE);
                },
                500
            );
            deadline = setTimeout(
                [&](){
                    clear_event(timer);
                    failed = true;
                },
                5000
            );

        },
        0
    );
    auto& table = tcp_port_table[k];

    await(
        [&table, &failed](){
            return table->get_SYN_ACK || failed;
        }
    );
    if(failed){
        setTimeout(
            [&, PORT](){
                tcp_port_table.erase(k);
                l = true;
            },
            0
        );
        return 0;
    }
    setTimeout(
        [&, deadline, timer, PORT](){
            clear_event(deadline);
            clear_event(timer);
            auto &table = tcp_port_table[k];
            table->state = 1;
            table->cwnd = MSS;
            table->duplicate_count = 0;
            table->dest_rwnd = RWND_MAX;
            table->ssthresh = THRESHOLD;
            l = true;
        },
        0
    );
    await(
        [&](){
            return l;
        }   
    );
    printf("Establish Link Successfully!\n====slow start====\n");
    key_table[PORT] = key{PORT, Server_port};

    return PORT;
}

uint16_t accept(const char* Server_IP, uint16_t Server_port){
    printf("Server listen...\n");
    await(
        [&](){
            return getSYN;
        }
    );

    key k;
    k.s = Server_port;
    k.d = client_port;

    auto& table = tcp_port_table[k];
    uint16_t port = client_port;

    bool l = false;
    await(
        [&](){
            return get3wayACK || syn_ack_failed;
        }
    );

    if(syn_ack_failed){
        setTimeout(
            [Server_port, k](){
                tcp_port_table.erase(k);
                getSYN = false;
                get3wayACK = false;
            },
            0
        );
        return 0;
    }
    setTimeout(
        [&](){
            getSYN = false;
            get3wayACK = false;
            l = true;
        },
        0
    );
    printf("Establish Link Successfully!\n");

    await([&](){return l;});

    return port;
}

int recv(uint16_t port, char* buf, int size){
    int buf_index = 0;
    bool end = false;
    auto& table = tcp_port_table[key_table[port]];

    cond_event(
        [&, table](){
            //printf("before recv buf size: %d, remain: %d\n", table->recv_tool->size(), table->recv_tool->remain());
            //while(table->expectSeqNum > table->lastByteRead){
            //    table->recv_tool->read(buf+buf_index, 1, table->lastByteRead);
            //    //table->recv_tool->erase(table->lastByteRead, 1);
            //    buf_index++;
            //    table->lastByteRead++;
            //    table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());
            //    if(buf_index == size || (table->recv_tool->size() == 0)){
            //        end = true;
            //        break;
            //    }
            //}
            int r_size = std::min(size, (int)table->expectSeqNum-(int)table->lastByteRead);
            table->recv_tool->read(buf+buf_index, r_size, table->lastByteRead);
            buf_index += r_size;
            table->lastByteRead += r_size;
            table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());
            end = true;
            //printf("after recv buf size: %d, remain: %d\n", table->recv_tool->size(), table->recv_tool->remain());
        },
        [&](){
            //printf("expect: %d, last read: %d, start: %d, end: %d, size:%d, remain: %d\n", table->expectSeqNum, table->lastByteRead, table->recv_tool->start, table->recv_tool->end, table->recv_tool->size(), table->recv_tool->remain());
            return (table->expectSeqNum > table->lastByteRead);}
    );
    await([&](){
        //std::cout << "recv wait\n"; 
        return end;}
    ); 
    
    return buf_index;
}

void send(uint16_t port, const char* buf, int size){
    auto& table = tcp_port_table[key_table[port]];

    sender(table, buf, size);
}

int read_till_ln(uint16_t port, char* buf){
    auto& table = tcp_port_table[key_table[port]];
    int buf_index = 0;
    bool end = false;

    
    cond_event(
        [&, table](){
            while(table->expectSeqNum > table->lastByteRead){
                table->recv_tool->read(buf+buf_index, 1, table->lastByteRead);
                //table->recv_tool->erase(table->lastByteRead, 1);
                buf_index++;
                table->lastByteRead++;
                table->rwnd = std::min(RWND_MAX, table->recv_tool->remain());
                if(buf[buf_index-1] == '\n'){
                    end = true;
                    break;
                }
            }
        },
        [=](){  return table->expectSeqNum > table->lastByteRead;}
    );
    await([&](){return end;});

    return buf_index;
}

void send_poisson(const char* IP, const char* data, int size, double loss_rate){
    if(loss_rate != 0 && (int)(m_unif(m_generator)*(int)(1/loss_rate))+1 == 1){
        printf("******LOSS packet: seq_num = %d, ack_num: %d\n", reinterpret_cast<const segment*>(data)->header.seq, reinterpret_cast<const segment*>(data)->header.ack);
        return; 
    }
    std::shared_ptr<char> send_data(new char[size]);
    memcpy(send_data.get(), data, size);
    setTimeout(
        [=](){udp_send(IP, send_data.get(), size);},
        20
    );
}