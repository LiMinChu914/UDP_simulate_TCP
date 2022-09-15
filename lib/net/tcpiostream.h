#ifndef __TCPIOSTREAM_H__
#define __TCPIOSTREAM_H__

#include <cstdint>
#include <map>
#include <string>
#include <memory>
#include "../tool/section_control.h"
#include "../tool/buffer.h"
#include <cstring>
#include "../tool/async.h"

#define MSS 1024
//#define MSS 200
#define THRESHOLD 20000
//#define THRESHOLD 800
#define RECV_Buf_SIZE 512 * 1024
//#define RECV_Buf_SIZE 2000

#define SEND_BUF_SIZE 1024 * 1024
//#define SEND_BUF_SIZE 100000
#define RWND_MAX 65535
//#define RWND_MAX 2000

#define TIMEOUT 2000
#define LOSS_RATE 0.001

#define print_loss 0

typedef struct {
    std::string Dest_IP;
    uint16_t Dest_port;
    uint16_t Source_port;

    int get_SYN_ACK;

    int state;
    uint32_t nextSeqNum;
    uint32_t cwnd;
    uint32_t rwnd;
    uint32_t sendEnd;
    uint32_t sendBase;
    uint32_t expectSeqNum;
    uint32_t lastByteRead;
    uint32_t dest_rwnd;
    uint32_t ssthresh;
    int duplicate_count;
    //for delay ack
    uint64_t delayAckTimer;
    uint64_t senderTimer;

    char send_buf[SEND_BUF_SIZE];
    char recv_buf[RECV_Buf_SIZE];
    std::shared_ptr<buffer_tool> send_tool;
    std::shared_ptr<buffer_tool> recv_tool;

    secCont recvControl;

    uint64_t debugger;
    struct {
        int get_SYN_ACK;
        int state;
        uint32_t nextSeqNum;
        uint32_t cwnd;
        uint32_t rwnd;
        uint32_t sendEnd;
        uint32_t sendBase;
        uint32_t expectSeqNum;
        uint32_t lastByteRead;
        uint32_t dest_rwnd;
        uint32_t ssthresh;
        int duplicate_count;
        // for delay ack
        uint64_t delayAckTimer;
        uint64_t senderTimer;
        // send buffer
        int s_start;
        int s_end;
        int s_size;
        int s_remain;
        // receiver buffer
        int r_start;
        int r_end;
        int r_size;
        int r_remain;
    } ds; // debugger state
    void enable_debugger() {
        memset(&ds, 0, sizeof(ds));
        debugger = setInterval([this]() {
            // state check
            bool check_get_SYN_ACK     = ds.get_SYN_ACK     != get_SYN_ACK    ;
            bool check_state           = ds.state           != state          ;
            bool check_nextSeqNum      = ds.nextSeqNum      != nextSeqNum     ;
            bool check_cwnd            = ds.cwnd            != cwnd           ;
            bool check_rwnd            = ds.rwnd            != rwnd           ;
            bool check_sendEnd         = ds.sendEnd         != sendEnd        ;
            bool check_sendBase        = ds.sendBase        != sendBase       ;
            bool check_expectSeqNum    = ds.expectSeqNum    != expectSeqNum   ;
            bool check_lastByteRead    = ds.lastByteRead    != lastByteRead   ;
            bool check_dest_rwnd       = ds.dest_rwnd       != dest_rwnd      ;
            bool check_ssthresh        = ds.ssthresh        != ssthresh       ;
            bool check_duplicate_count = ds.duplicate_count != duplicate_count;
            // for delay ack
            bool check_delayAckTimer   = ds.delayAckTimer   != delayAckTimer  ;
            bool check_senderTimer     = ds.senderTimer     != senderTimer    ;
            // send buffer
            bool check_s_start         = ds.s_start         != send_tool->start   ;
            bool check_s_end           = ds.s_end           != send_tool->end     ;
            bool check_s_size          = ds.s_size          != send_tool->size()  ;
            bool check_s_remain        = ds.s_remain        != send_tool->remain();
            // receiver buffer
            bool check_r_start         = ds.r_start         != recv_tool->start   ;
            bool check_r_end           = ds.r_end           != recv_tool->end     ;
            bool check_r_size          = ds.r_size          != recv_tool->size()  ;
            bool check_r_remain        = ds.r_remain        != recv_tool->remain();
            if (check_get_SYN_ACK    ) ds.get_SYN_ACK     = get_SYN_ACK    ;
            if (check_state          ) ds.state           = state          ;
            if (check_nextSeqNum     ) ds.nextSeqNum      = nextSeqNum     ;
            if (check_cwnd           ) ds.cwnd            = cwnd           ;
            if (check_rwnd           ) ds.rwnd            = rwnd           ;
            if (check_sendEnd        ) ds.sendEnd         = sendEnd        ;
            if (check_sendBase       ) ds.sendBase        = sendBase       ;
            if (check_expectSeqNum   ) ds.expectSeqNum    = expectSeqNum   ;
            if (check_lastByteRead   ) ds.lastByteRead    = lastByteRead   ;
            if (check_dest_rwnd      ) ds.dest_rwnd       = dest_rwnd      ;
            if (check_ssthresh       ) ds.ssthresh        = ssthresh       ;
            if (check_duplicate_count) ds.duplicate_count = duplicate_count;
            if (check_delayAckTimer  ) ds.delayAckTimer   = delayAckTimer  ;
            if (check_senderTimer    ) ds.senderTimer     = senderTimer    ;
            if (check_s_start        ) ds.s_start         = send_tool->start   ;
            if (check_s_end          ) ds.s_end           = send_tool->end     ;
            if (check_s_size         ) ds.s_size          = send_tool->size()  ;
            if (check_s_remain       ) ds.s_remain        = send_tool->remain();
            if (check_r_start        ) ds.r_start         = recv_tool->start   ;
            if (check_r_end          ) ds.r_end           = recv_tool->end     ;
            if (check_r_size         ) ds.r_size          = recv_tool->size()  ;
            if (check_r_remain       ) ds.r_remain        = recv_tool->remain();
            bool changed =
                check_get_SYN_ACK    |
                check_state          |
                check_nextSeqNum     |
                check_cwnd           |
                check_rwnd           |
                check_sendEnd        |
                check_sendBase       |
                check_expectSeqNum   |
                check_lastByteRead   |
                check_dest_rwnd      |
                check_ssthresh       |
                check_duplicate_count|
                check_delayAckTimer  |
                check_senderTimer    |
                check_s_start        |
                check_s_end          |
                check_s_size         |
                check_s_remain       |
                check_r_start        |
                check_r_end          |
                check_r_size         |
                check_r_remain       ;
            if (changed) {
                std::cout << "//////////////////////////////// Debugger log ////////////////////////////////" << std::endl;
                std::cout << (check_get_SYN_ACK     ? ">" : " ") << "get_SYN_ACK    : " << ds.get_SYN_ACK     << std::endl;
                std::cout << (check_state           ? ">" : " ") << "state          : " << ds.state           << std::endl;
                std::cout << (check_nextSeqNum      ? ">" : " ") << "nextSeqNum     : " << ds.nextSeqNum      << std::endl;
                std::cout << (check_cwnd            ? ">" : " ") << "cwnd           : " << ds.cwnd            << std::endl;
                std::cout << (check_rwnd            ? ">" : " ") << "rwnd           : " << ds.rwnd            << std::endl;
                std::cout << (check_sendEnd         ? ">" : " ") << "sendEnd        : " << ds.sendEnd         << std::endl;
                std::cout << (check_sendBase        ? ">" : " ") << "sendBase       : " << ds.sendBase        << std::endl;
                std::cout << (check_expectSeqNum    ? ">" : " ") << "expectSeqNum   : " << ds.expectSeqNum    << std::endl;
                std::cout << (check_lastByteRead    ? ">" : " ") << "lastByteRead   : " << ds.lastByteRead    << std::endl;
                std::cout << (check_dest_rwnd       ? ">" : " ") << "dest_rwnd      : " << ds.dest_rwnd       << std::endl;
                std::cout << (check_ssthresh        ? ">" : " ") << "ssthresh       : " << ds.ssthresh        << std::endl;
                std::cout << (check_duplicate_count ? ">" : " ") << "duplicate_count: " << ds.duplicate_count << std::endl;
                std::cout << (check_delayAckTimer   ? ">" : " ") << "delayAckTimer  : " << ds.delayAckTimer   << std::endl;
                std::cout << (check_senderTimer     ? ">" : " ") << "senderTimer    : " << ds.senderTimer     << std::endl;
                std::cout << (check_s_start         ? ">" : " ") << "s_start        : " << ds.s_start         << std::endl;
                std::cout << (check_s_end           ? ">" : " ") << "s_end          : " << ds.s_end           << std::endl;
                std::cout << (check_s_size          ? ">" : " ") << "s_size         : " << ds.s_size          << std::endl;
                std::cout << (check_s_remain        ? ">" : " ") << "s_remain       : " << ds.s_remain        << std::endl;
                std::cout << (check_r_start         ? ">" : " ") << "r_start        : " << ds.r_start         << std::endl;
                std::cout << (check_r_end           ? ">" : " ") << "r_end          : " << ds.r_end           << std::endl;
                std::cout << (check_r_size          ? ">" : " ") << "r_size         : " << ds.r_size          << std::endl;
                std::cout << (check_r_remain        ? ">" : " ") << "r_remain       : " << ds.r_remain        << std::endl;
            }
        }, 10);
    }
} tcp_link_info;

struct key{
    uint16_t s;
    uint16_t d;
    bool operator<(const key& c) const {
        return (static_cast<uint32_t>(s) << 16 | static_cast<uint32_t>(d)) < (static_cast<uint32_t>(c.s) << 16 | static_cast<uint32_t>(c.d));
    }
};

extern std::map<struct key, std::shared_ptr<tcp_link_info> > tcp_port_table;

extern void (*tcp_ostream)(const char* IP, const char* data, int size, double loss_rate);

void tcp_istream(const char* IP, const char* data, int size);

void set_tcp_ostream(void (*work)(const char* IP, const char* data, int size, double loss_rate));

#endif