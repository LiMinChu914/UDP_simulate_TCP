#include"tcp_duplex.h"
#include"tcpiostream.h"
#include"receiver.h"
#include"sender.h"
#include"../tool/async.h"


void tcp_duplex(std::shared_ptr<segment> pack, int size){
    key k = {pack->header.dest, pack->header.source};
    auto& table = tcp_port_table[k];

    if(!print_loss)
        printf("\tReceive a packet ( seq_num = %d, ack_num = %d )\n", pack->header.seq, pack->header.ack);
    bool not_send_ack = false;

    if(pack->header.ACK == 1){
        //new ACK
        if(pack->header.ack > table->sendBase && pack->header.ack <= table->nextSeqNum){
            table->dest_rwnd = pack->header.rwnd;
            //sendBase move
            //printf("before mvoe start send buf size: %d, remain: %d, base: %d\n", table->send_tool->size(), table->send_tool->remain(), table->sendBase);
            table->send_tool->move_start(pack->header.ack-table->sendBase, table->sendBase);
            table->sendBase = pack->header.ack;
            //printf("after move start send buf size: %d, remain: %d, base: %d\n", table->send_tool->size(), table->send_tool->remain(), table->sendBase);
            //printf("base: %d, next: %d, send buf size: %d, state; %d\n", table->sendBase, table->nextSeqNum, table->send_tool->size(), table->state);

            if(table->state == 1){
                table->cwnd += MSS;
                if(table->cwnd >= table->ssthresh){
                    printf("====congestion avoidance====\n");
                    table->state = 2;
                }
                table->duplicate_count = 0;
            }
            else if(table->state == 2){
                table->cwnd += (int)(MSS*((double)MSS/(double)table->cwnd));
                table->duplicate_count = 0;
            }
            else if(table->state == 3){

                printf("new ACK\n====congestion avoidance====\n");
                table->state = 2;
                table->cwnd = table->ssthresh;
                table->duplicate_count = 0;
            }

            //start timer
            if(table->senderTimer != -1){
                clear_event(table->senderTimer);
                //printf("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n START TIMER \n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
                table->senderTimer = setInterval(
                    [table](){
                        table->ssthresh = table->cwnd / 2;
                        table->cwnd = MSS;
                        table->duplicate_count = 0;
                        table->state = 1;
                        printf("TIMEOUT\n====slow start=====\n");
                        fast_retransmit(table);
                        // && table->send_tool->size() == 0
                        if(table->sendBase == table->nextSeqNum && table->send_tool->size() == 0){
                            //stop timer
                            //printf("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n STOP TIMER \n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
                            clear_event(table->senderTimer);
                            table->senderTimer = -1;
                        }
                    },
                    TIMEOUT
                );
            }
            //send new data
            not_send_ack = send_new(table);

        }
        // duplicate ACK
        //table->send_tool->size() > 0 && 
        else if(pack->header.ack == table->sendBase){
            //printf("***table send base: %d\n", table->sendBase);
            table->dest_rwnd = pack->header.rwnd;
            table->duplicate_count++;

            if(table->state == 3){
                table->cwnd += MSS;
            
                //send new data
                not_send_ack = send_new(table);
            }

            if(table->duplicate_count == 3){
                printf("receive three duplicated ACK\n");
                printf("====fast recovery====\n");

                table->ssthresh = table->cwnd/2;
                table->cwnd = table->ssthresh + 3*MSS;
                table->state = 3;
            
                //fast retransmit
                fast_retransmit(table);
                //start timer
                if(table->senderTimer != -1){
                    //printf("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n START TIMER \n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
                    clear_event(table->senderTimer);
                    //std::shared_ptr<int> id = std::make_shared<int>();
                    table->senderTimer = setInterval(
                        [table](){
                            table->ssthresh = table->cwnd / 2;
                            table->cwnd = MSS;
                            table->duplicate_count = 0;
                            table->state = 1;
                            fast_retransmit(table);
                            if(table->sendBase == table->nextSeqNum && table->send_tool->size() == 0){
                                //stop timer
                                //printf("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n STOP TIMER \n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
                                clear_event(table->senderTimer);
                                table->senderTimer = -1;
                            }
                        },
                        TIMEOUT
                    );
                }
            }
        }
        
    }


    receiver(pack, size, not_send_ack);

}
