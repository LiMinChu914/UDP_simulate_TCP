#include "buffer.h"
#include <string.h>
#include <sstream>
#include<iostream>



buffer_tool::buffer_tool(char* in, int cap){
    buf = in;
    start = 0;
    end = 0;
    capacity = cap;
}

void buffer_tool::clear(int pos){
    start = end = pos%capacity;
}

int buffer_tool::write(const char* data, int size, int pos){
    int p = pos%capacity;

    if(p < start)
        p += capacity;
    int write_end = p + size;
    int pseudo_end = (end < start) ? end+capacity: end;

    if(size-pseudo_end+p > remain()){
        printf("WRITE BUF ERROR\n");
        return -1;
    }
    //printf("cap: %d, p: %d, size: %d, pseudo_end: %d, write_end: %d\n", capacity, p, size, pseudo_end, write_end);
    end = (pseudo_end < write_end)? write_end%capacity: pseudo_end%capacity;

    if(pos%capacity + size > capacity){
        memcpy(buf+pos%capacity, data, capacity-pos%capacity);
        memcpy(buf, data+(capacity-pos%capacity), size-(capacity-pos%capacity));
    }
    else{
        memcpy(buf+pos%capacity, data, size);
    }
    
    return 0;
}

int buffer_tool::read(char* out, int size, int pos){
    int p = pos%capacity;

    if(p < start)
        p += capacity;
    
    int read_end = p + size;
    int pseudo_end = (end < start) ? end+capacity: end;
    //printf("read_end : %d, pseudo_end : %d\n", read_end, pseudo_end);

    if(read_end > pseudo_end)
        return -1;

    start = read_end % capacity;

    if(pos%capacity + size > capacity){
        memcpy(out, buf+pos%capacity, capacity-pos%capacity);
        memcpy(out+capacity-pos%capacity, buf, size-(capacity-pos%capacity));
    }
    else{
        memcpy(out, buf+pos%capacity, size);
    }
    
    return 0;
}

int buffer_tool::copy(char* out, int size, int pos){
    /*int p = pos%capacity;

    if(p < start)
        p += capacity;
    
    int read_end = p + size;
    int pseudo_end = (end < start) ? end+capacity: end;
    printf("read_end : %d, pseudo_end : %d\n", read_end, pseudo_end);

    if(read_end > pseudo_end)
        return -1;

    start = read_end % capacity;*/

    if(pos%capacity + size > capacity){
        memcpy(out, buf+pos%capacity, capacity-pos%capacity);
        memcpy(out+capacity-pos%capacity, buf, size-(capacity-pos%capacity));
    }
    else{
        memcpy(out, buf+pos%capacity, size);
    }
    
    return 0;
}

int buffer_tool::move_start(int size, int pos){
    int p = pos%capacity;

    if(p < start)
        p += capacity;
    
    int read_end = p + size;
    int pseudo_end = (end < start) ? end+capacity: end;

    if(read_end > pseudo_end)
        return -1;

    start = read_end % capacity;

    return 0;
}

std::string buffer_tool::to_string() const {
    /*
    x x x 1 2 3 4 5 6 x x x 
    <x> x  x  1  2  3  4  5 >6< x  x  x 
    #x# x  x  1  2  3  4  5  6  x  x  x 
    */
    std::stringstream ss;
    for(int i = 0; i < capacity; i++){
        if(i == start && i == end){
            ss << "#" << buf[i] << "#";
        }
        else if(i == start){
            ss << "<" << buf[i] << ">";
        }
        else if(i == end){
            ss << ">" << buf[i] << "<";
        }
        else{
            ss << " " << buf[i] << " ";
        }
    }

    return ss.str();
}

/*int main(){
    int a;
    char b[10];
    memset(b, '@', 10);
    buffer_tool k(b, 10);
    k.clear(3);
    std::cout << "size: " << k.size() << "\t | " << k.to_string() << std::endl;
    a = k.write("123", 3, 5);
    std::cout << a << " size: " << k.size() << "\t | " << k.to_string() << std::endl;
    a = k.write("346", 3, 3);
    std::cout << a << " size: " << k.size() << "\t | " << k.to_string() << std::endl;
    a = k.write("7892", 4, 8);
    std::cout << a << " size: " << k.size() << "\t | " << k.to_string() << std::endl;
    char d[10];
    k.read(d, 3, 4);
    std::cout << "size: " << k.size() << "\t | " << k.to_string() << std::endl;
    for(int i = 0; i < 4; i++)
        std::cout << d[i] << " ";
    std::cout << "\n";
}*/
//int buffer_tool::write(const char* data, int size, int pos) {
//    if (size == 0) return true;
//    int dummy_end = end;
//    if (start > end)
//        dummy_end = end + capacity;
//    pos %= capacity;
//    if (pos < start) pos += capacity;
//    int write_end = pos + size;
//    if (write_end > remain() + dummy_end) return false;
//    int _pos = pos %= capacity, _write_end = _pos + size;
//    if (_write_end >= capacity) {
//        memcpy(buf + _pos, data, capacity - _pos);
//        memcpy(buf, data + capacity - _pos, size - capacity + _pos);
//    }
//    else {
//        memcpy(buf + _pos, data, size);
//    }
//    if (write_end > dummy_end)
//        end = write_end % capacity;
//    return true;
//}
//
//int buffer_tool::read(char* out, int size, int pos) {
//    pos %= capacity;
//    int dummy_end = end;
//    if (start > end)
//        dummy_end = end + capacity;
//    if (pos + size > dummy_end)
//        return false;
//    int read_end = pos + size;
//    if (read_end >= capacity) {
//        memcpy(out, buf + pos, capacity - pos);
//        memcpy(out + capacity - pos, buf, size - capacity + pos);
//    }
//    else {
//        memcpy(out, buf + pos, size);
//    }
//    return true;
//}
//
//int buffer_tool::erase(int pos, int size) {
//    pos %= capacity;
//    int dummy_end = end;
//    if (start > end)
//        dummy_end = end + capacity;
//    if (pos + size > dummy_end)
//        return false;
//    int read_end = pos + size;
//    /*if (read_end >= _capacity) {
//        memset(elems + pos, '@', _capacity - pos);
//        memset(elems, '@', size - _capacity + pos);
//    }
//    else {
//        memset(elems + pos, '@', size);
//    }*/
//    start = (pos + size) % capacity;
//    return true;
//}
