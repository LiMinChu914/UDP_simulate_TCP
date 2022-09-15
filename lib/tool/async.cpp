#include"async.h"
#include<map>
#include<condition_variable>
#include<chrono>
#include<thread>

#include"EventDispatcher.h"

#define USE_EVENTDISPATCHER
#ifdef USE_EVENTDISPATCHER

void clear_event(uint64_t event) {
    EventDispatcher::clearTimeout(event);
}

uint64_t cond_event(std::function<void()> work, std::function<bool()> cond, int valid) {
    return EventDispatcher::setConditional(std::move(work), std::move(cond));
}

uint64_t setTimeout(std::function<void()> work, uint64_t wait) {
    return EventDispatcher::setTimeout(std::move(work), wait);
}

uint64_t setInterval(std::function<void()> work, uint64_t delay) {
    return EventDispatcher::setInterval(std::move(work), delay);
}

void await(std::function<bool()> cond) {
    EventDispatcher::await(std::move(cond));
}

void event_handler() {
    EventDispatcher::startEventDispatcher();
}

#else

struct event_info{
    int valid;
    std::function<void()> work;
    std::function<bool()> cond;

};
using wrap = std::shared_ptr<struct event_info>;


std::map<uint16_t, wrap> event_table;
std::mutex event_mutex;

void push_event(wrap& event, uint64_t& id){
    std::lock_guard<std::mutex> lock(event_mutex);
    id++;
    event_table.insert({id, event});
}

void clear_event(uint64_t event){
    std::lock_guard<std::mutex> lock(event_mutex);
    if(event_table.count(event) != 0)
        event_table.erase(event);
}

uint64_t cond_event(std::function<void()> work, std::function<bool()> cond, int valid){
    static uint64_t id = 0;

    wrap info = std::make_shared<event_info>();
    info->valid = valid;
    info->work = move(work);
    info->cond = move(cond);
    
    push_event(info, id);

    return id;
}

uint64_t setTimeout(std::function<void()> work, uint64_t wait){
    auto exec_time = steady_clock::now();
    return cond_event(work, [=](){ return (duration_cast<milliseconds>)(steady_clock::now()-exec_time).count() >= wait; });
}

uint64_t setInterval(std::function<void()> work, uint64_t delay){
    std::shared_ptr<std::chrono::_V2::steady_clock::time_point> exec_time = std::make_shared<steady_clock::time_point>();
    std::shared_ptr<int> wait_count = std::make_shared<int>(1);
    *exec_time = steady_clock::now();

    return cond_event(
        [work, exec_time, wait_count]() {
            {
            std::lock_guard<std::mutex> lock(event_mutex);
            *wait_count = event_table.size();
            }
            work();
            *exec_time = steady_clock::now();
        },
        [=](){ return --*wait_count <= 0 && (duration_cast<milliseconds>)(steady_clock::now() - *exec_time).count() >= delay; },
        1
    );

}

void await(std::function<bool()> cond){
    std::shared_ptr<std::mutex> await_mutex = std::make_shared<std::mutex>();
    std::shared_ptr<std::condition_variable> cond_var = std::make_shared<std::condition_variable>();
    std::shared_ptr<std::unique_lock<std::mutex>> lock = std::make_shared<std::unique_lock<std::mutex>>(*await_mutex);

    bool condition = false;

    uint64_t timer = cond_event(
        [=, &condition, &timer](){
            timer = setInterval(
                [=](){
                    cond_var->notify_all();
                },
                10
            );
            condition = true;
        },
        cond
    );

    while(!condition) cond_var->wait(*lock);

    clear_event(timer);
}

void event_handler(){
    auto& tcp_info = event_table;

    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::function<void()> callback;
        {
        std::lock_guard<std::mutex> lock(event_mutex);
        for(auto& event: tcp_info){

            if((event.second->cond)()){
                callback = event.second->work;
                if(!event.second->valid)
                    event_table.erase(event.first);
                break;
            }
        }
        }
        if(callback)
            callback();
    }
}

#endif