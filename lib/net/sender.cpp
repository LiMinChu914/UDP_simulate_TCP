#include"sender.h"
#include"../tool/async.h"
#include"packet.h"
//printf("===========wait==========|size: %d, remain: %d\n", table->send_tool->size(), table->send_tool->remain());
void sender(std::shared_ptr<tcp_link_info> table, const char* in, int size){
    bool complete_write = false;
    cond_event([=, &complete_write](){
        //printf("before send buf size: %d, remain: %d, start: %d, end: %d\n", table->send_tool->size(), table->send_tool->remain(), table->send_tool->start, table->send_tool->end);
        table->send_tool->write(in, size, table->sendBase+table->send_tool->size());
        //printf("after send buf size: %d, remain: %d, start: %d, end: %d\n", table->send_tool->size(), table->send_tool->remain(), table->send_tool->start, table->send_tool->end);
        send_new(table);
        complete_write = true;
    }, [=](){ return table->send_tool->remain() > size; });
    await(
        [&](){return complete_write;}
    );
}

bool send_new(std::shared_ptr<tcp_link_info> table){
    bool success = false;
    //printf("send new: base: %d, send buf size: %d, next: %d, base+size: %d\n", table->sendBase, table->send_tool->size(), table->nextSeqNum, table->sendBase+table->send_tool->size());
    if(table->sendBase+table->send_tool->size()-table->nextSeqNum > 0){
        int windowSize = std::min(table->dest_rwnd, table->cwnd);
        int send_end = (table->sendBase+windowSize > table->sendBase+table->send_tool->size())? table->sendBase+table->send_tool->size(): table->sendBase+windowSize;
        //printf("send end: %d, wnd size: %d, cwnd: %d, dest rwnd: %d, timer: %d\n", send_end, windowSize, table->cwnd, table->dest_rwnd, table->senderTimer);
        //printf("next: %d, base: %d\n", table->nextSeqNum, table->sendBase);
        while(table->nextSeqNum < send_end){
            success = true;
            int pack_length = (MSS < send_end-table->nextSeqNum)? MSS: send_end-table->nextSeqNum;

            std::shared_ptr<segment> send_pack((segment*)(operator new(sizeof(segment)+pack_length)));
            set_packetH(send_pack, table->Source_port, table->Dest_port, table->nextSeqNum, table->expectSeqNum, 5, 1, 0, 0, table->rwnd, 0);
            table->send_tool->copy(reinterpret_cast<char*>(send_pack->data), pack_length, table->nextSeqNum);

            tcp_ostream(table->Dest_IP.c_str(), reinterpret_cast<const char*>(&*send_pack), pack_length+sizeof(segment), LOSS_RATE);
            if(!print_loss)
                printf("cwnd = %d, rwnd = %d, threshold = %d\n\tSend a packet at %d\n", table->cwnd, table->rwnd, table->ssthresh, send_pack->header.seq);
            //start timer
            if(table->sendBase == table->nextSeqNum){
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
                else{
                    //printf("\n &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n START TIMER \n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
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

            table->nextSeqNum += pack_length;
        }

    }
    return success;
}

bool fast_retransmit(std::shared_ptr<tcp_link_info> table){
    uint32_t retransmit_index = table->sendBase;
    bool success = false;

    //uint64_t end = std::min(table->nextSeqNum, table->sendBase+std::min(table->cwnd, table->dest_rwnd));
    uint64_t end = table->nextSeqNum;
    while(retransmit_index < end){
        success = true;
        int pack_length = (MSS < end-retransmit_index)? MSS: end-retransmit_index;

        std::shared_ptr<segment> send_pack((segment*)(operator new(sizeof(segment)+pack_length)));
        set_packetH(send_pack, table->Source_port, table->Dest_port, retransmit_index, table->expectSeqNum, 5, 1, 0, 0, table->rwnd, 0);
        table->send_tool->copy(reinterpret_cast<char*>(send_pack->data), pack_length, retransmit_index);

        tcp_ostream(table->Dest_IP.c_str(), reinterpret_cast<const char*>(&*send_pack), pack_length+sizeof(segment), LOSS_RATE);
        if(!print_loss)
            printf("cwnd = %d, rwnd = %d, threshold = %d\n\tFast retransmit a packet at %d\n", table->cwnd, table->rwnd, table->ssthresh, send_pack->header.seq);

        retransmit_index += pack_length;
    }

    return success;
}