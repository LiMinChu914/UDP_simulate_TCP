#include <iostream>
#include <fstream>
#include <thread>
#include <string.h>
#include <vector>
#include "lib/net/udp.h"
#include "lib/net/packet.h"
#include "lib/tool/async.h"
#include "lib/net/tcpiostream.h"
#include "lib/net/tcp.h"
#include "lib/net/sender.h"



void get_video(uint16_t port, const char* video_path, int thread_num){
    char buf[100];
    std::string path = "video "+std::string(video_path)+"\n";
    send(port, reinterpret_cast<const char*>(path.c_str()), path.length());
    int buf_index = read_till_ln(port, buf);
    std::string get = "";
    for(int i = 0; i < buf_index; i++){
        get += buf[i];
    }
    //std::cout << get << std::endl;
    if (get.substr(0, 5) == "Can't")
        return;
    
    std::string s = "";
    for(int i = 0; i < buf_index; i++)
        if(get[i] != '\n')
            s += get[i];
    //std::cout << "Video file size: " << s << " bytes" << std::endl;
    int file_size = atoi(s.c_str());
    std::string out_path = "../vid_" + std::string(video_path).substr(3,1) + "_thread" + std::to_string(thread_num) + ".mp4";
    std::ofstream output(out_path, std::ios::binary);
    char* buffer = new char[64000];
    while (file_size > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        int recv_size = recv(port, buffer, std::min(64000, file_size));
        file_size -= recv_size;
        output.write(buffer, recv_size);
        //std::cout << "file_size: " << file_size << std::endl;
    }
    delete[] buffer;
    output.close();
}



void calculate(uint16_t port, std::string op, int a, int b, int thread_num){
    char buf[100];
    std::string msg;
    if(op == "sqrt")
        msg = op + " " + std::to_string(a) + "\n";
    else
        msg = op + " " + std::to_string(a) + " " + std::to_string(b) + "\n";

    send(port, reinterpret_cast<const char*>(msg.c_str()), msg.length());
    int size = read_till_ln(port, buf);
    std::string out_path = "../cal_thread" + std::to_string(thread_num) + ".txt";
    std::ofstream output(out_path, std::ios::binary);

    std::string ans = "";
    for(int i = 0; i < size; i++)
        ans += buf[i];
    std::string ret = op + " " + std::to_string(a) + " " + std::to_string(b) + " = " + ans;
    output.write(reinterpret_cast<const char*>(ret.c_str()), ret.length());
    output.close();
}

std::string op[6] = {"add", "sub", "mul", "div", "pow", "sqrt"};

int main(int argc, char** argv){
    std::thread handler(event_handler);
    set_recv_callback(tcp_istream);
    set_tcp_ostream(send_poisson);

    client_init(argv[1], atoi(argv[2]));
    std::thread udp(udp_recv);

    std::vector<std::thread> ts;
    for (int i = 0; i < 1; i++) {
        ts.push_back(std::thread([&, i]() {
            uint16_t sock = connect(argv[1], atoi(argv[2]));
            for(int i = 0; i < 8; i++){
                std::string path = "../" + std::to_string(i+1) + ".mp4";
                get_video(sock, reinterpret_cast<const char*>(path.c_str()), i);
            }
            //get_video(sock, "../1.mp4", i);
            
            /*uint16_t sock = connect(argv[1], atoi(argv[2]));
            if(i % 3 == 0){
                std::string path = "../4.mp4";
                get_video(sock, reinterpret_cast<const char*>(path.c_str()), i);
                calculate(sock, op[2], 6, 4, i);
            }
            else{
                calculate(sock, op[i%6+1], 6, 4, i);
            }*/
        }));
    }

    for(auto& t: ts)
        t.join();

    handler.join();
    udp.join();
    return 0;
}
