#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <string>

class buffer_tool{
public:
    buffer_tool(char* in, int cap);
    void clear(int pos);
    int write(const char* data, int size, int pos);
    int read(char* out, int size, int pos);
    int copy(char* out, int size, int pos);
    int move_start(int size, int pos);
    bool empty() const { return start == end; }
    int size() const {return (end < start) ? capacity-start+end: end-start; }
    int remain() const {return capacity-size()-1; }
    //int erase(int pos, int size);
    
    std::string to_string() const;

//private:
    char* buf;
    int start;
    int end;
    int capacity;
};

#endif