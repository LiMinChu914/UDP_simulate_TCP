#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string.h>
#include <string>
#include "lib/net/udp.h"
#include "lib/tool/async.h"
#include "lib/net/tcpiostream.h"
#include "global_var.h"
#include "lib/net/tcp.h"
#include "lib/net/sender.h"
#include <sstream>
#include <math.h>

#define SERV_PORT 8000
#define SERV_IP "127.0.0.1"


void send_vid(uint16_t port, const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        std::string reply = std::string() + "Can't find video file \"" + path + "\".\n";
        send(port, reinterpret_cast<const char*>(reply.c_str()), reply.length());
        return;
    }

    input.seekg(0, std::ios::end);
    int file_size = input.tellg();
    input.seekg(0, std::ios::beg);
    std::string vid = std::to_string(file_size) + "\n";
    send(port, reinterpret_cast<const char*>(vid.c_str()), vid.length());

    char* buffer = new char[64000];
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        input.read(buffer, 64000);
        send(port, buffer, input.gcount());
        //printf("file size: %d\n", file_size-=input.gcount());
        if (input.eof()) {
            break;
        }
    }
    delete[] buffer;
}

void server(uint16_t port){
    char buf[100];

    while(true){
        int buf_index = read_till_ln(port, buf);
        std::string get = "";
        for(int i = 0; i < buf_index; i++){
            get += buf[i];
        }
        std::stringstream ss;
        ss << get;
        std::string request;
        ss >> request;
        if(request == "add"){
            int a,b;
            ss >> a >> b;
            std::string ans = std::to_string(((double)a+b));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "sub"){
            int a,b;
            ss >> a >> b;
            std::string ans = std::to_string(((double)a-b));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "mul"){
            int a,b;
            ss >> a >> b;
            std::string ans = std::to_string(((double)a*b));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "div"){
            int a,b;
            ss >> a >> b;
            std::string ans = std::to_string(((double)a/b));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "pow"){
            int a,b;
            ss >> a >> b;
            std::string ans = std::to_string(pow(a,b));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "sqrt"){
            int a;
            ss >> a;
            std::string ans = std::to_string(sqrt(a));
            ans += "\n";
            send(port, reinterpret_cast<const char*>(ans.c_str()), ans.length());
        }
        else if(request == "video"){
            std::string path;
            ss >> path;
            send_vid(port, path);
        }
    }
}


int main(int argc, char** argv){
    std::thread handler(event_handler);
    set_recv_callback(tcp_istream);
    set_tcp_ostream(send_poisson);

    server_init(SERV_IP,SERV_PORT);
    std::thread udp(udp_recv);

    std::vector<std::thread> threads;
    int count = 0;

    while(true){
        uint16_t sock = accept("127.0.0.1", 8000);
        threads.push_back(std::thread(server, sock));
    }


    for(auto& th: threads){
        th.join();
    }
    handler.join();
    udp.join();
    return 0;
}
