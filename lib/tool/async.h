#ifndef __ASYNC_H__
#define __ASYNC_H__

#include<cstdint>
#include<memory>
#include<functional>
#include<mutex>
#include<ctime>


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;


void clear_event(uint64_t event);

uint64_t cond_event(std::function<void()> work, std::function<bool()> cond, int valid = 0);
uint64_t setTimeout(std::function<void()> work, uint64_t wait);

uint64_t setInterval(std::function<void()> work, uint64_t delay);

void await(std::function<bool()> cond);

void event_handler();

#endif