#include "EventDispatcher.h"

#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <memory>
#include <condition_variable>

namespace EventDispatcher {

using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using nanoseconds  = std::chrono::nanoseconds;
using time_point   = std::chrono::time_point<std::chrono::system_clock>;

std::thread::id system_tid;
std::thread::id owner_tid; // who owns this system now

timeoutID event_count = 1; // start from 1

std::mutex id_lock;

void run(); // run system

struct event {
    conditionalCallback cond;
    timeoutCallback callback;
    bool erase;
};

using event_ptr = std::shared_ptr<event>;

std::map<timeoutID, event_ptr> event_list;
std::mutex event_lock;

std::mutex system_mutex;

inline timeoutID set(timeoutCallback callback, conditionalCallback cond, bool erase) {
    id_lock.lock();
    timeoutID tid = event_count++;
    id_lock.unlock();
    event_ptr e = std::make_shared<event>();
    e->cond = std::move(cond);
    e->callback = std::move(callback);
    e->erase = erase;
    std::unique_lock<std::mutex> lock(event_lock);
    event_list[tid] = std::move(e);
    return tid;
}

timeoutID setTimeout(timeoutCallback callback, time_t delay) {
    time_point exec_time = std::chrono::system_clock::now() + milliseconds(delay);
    return set(
        std::move(callback),
        std::move([exec_time]() { return std::chrono::system_clock::now() >= exec_time; }),
        true);
}

timeoutID setInterval(timeoutCallback callback, time_t delay) {
    milliseconds _delay(delay);
    std::shared_ptr<time_point> exec_time = std::make_shared<time_point>(std::chrono::system_clock::now() + _delay);
    return set(
        std::move(callback),
        std::move([exec_time, _delay, delay]() {
            time_point* ptr = exec_time.get();
            bool cond = std::chrono::system_clock::now() >= *ptr;
            if (cond && delay > 0) *ptr += ((std::chrono::system_clock::now() - *ptr) / _delay + 1) * _delay;
            return cond;
        }),
        false);
}

timeoutID setConditional(timeoutCallback callback, conditionalCallback cond) {
    return set(std::move(callback), std::move(cond), true);
}

void clearTimeout(timeoutID tid) {
    std::unique_lock<std::mutex> lock(event_lock);
    if (event_list.count(tid) == 0)
        return;
    event_list.erase(tid);
}

void clearInterval(timeoutID tid) {
    std::unique_lock<std::mutex> lock(event_lock);
    if (event_list.count(tid) == 0)
        return;
    event_list.erase(tid);
}

// launch an event synchronized with worker
void setTimeoutSync(timeoutCallback callback, time_t delay) {
    if (delay > 0) {
        time_point exec_time = std::chrono::system_clock::now() + milliseconds(delay);
        while (exec_time > std::chrono::system_clock::now());
    }
    std::unique_lock<std::mutex> lock(system_mutex);
    owner_tid = std::this_thread::get_id();
    try {
        callback();
    } catch(...) {}
    owner_tid = system_tid;
}

void system_await(conditionalCallback callback);
void thread_await(conditionalCallback callback);

void await(conditionalCallback callback) {
    if (std::this_thread::get_id() == system_tid) {
        system_await(std::move(callback));
    }
    else {
        thread_await(std::move(callback));
    }
}

void system_await(conditionalCallback callback) {
    bool cond = false;
    setConditional([&]() {
        cond = true;
    }, std::move(callback));
    system_mutex.unlock();
    while (!cond) {
        run();
    }
    system_mutex.lock();
}
void thread_await(conditionalCallback callback) {
    std::mutex mu;
    std::unique_lock<std::mutex> lock(mu);
    std::condition_variable cv;
    bool cond = false;
    
    timeoutID timeout = setConditional([&]() {
        cond = true;
        cv.notify_all();
        timeout = setInterval([&]() {
            cv.notify_all();
        }, 1);
    }, std::move(callback));

    while (!cond) cv.wait(lock);

    SystemLock sys_lock = getSystemLock();
    sys_lock.lock();
    clearTimeout(timeout);
    sys_lock.unlock();
}

SystemLock::SystemLock() : locked(false) {}
SystemLock::~SystemLock() {
    if (locked) {
        owner_tid = system_tid;
        system_mutex.unlock();
    }
}
void SystemLock::lock() {
    std::thread::id this_id = std::this_thread::get_id();
    if (locked || this_id == owner_tid) return;
    system_mutex.lock();
    owner_tid = this_id;
    locked = true;
}
void SystemLock::unlock() {
    if (!locked) return;
    locked = false;
    owner_tid = system_tid;
    system_mutex.unlock();
}

SystemLock getSystemLock() {
    return {};
}

size_t todoCount() {
    if (std::this_thread::get_id() == system_tid)
        return event_list.size();
    std::unique_lock<std::mutex> lock(event_lock);
    return event_list.size();
}

int state;  // system state

void run() {
    bool has_event_occurs = false;
    timeoutID exec_event_tid = -1;
    event_ptr exec_event = nullptr;
    /* Lock system */
    system_mutex.lock();
    /* Lock event_list */ {
    std::unique_lock<std::mutex> lock(event_lock);
    for (auto& event_wrap : event_list) {
        if (!event_wrap.second->cond())
            continue;
        has_event_occurs = true;
        exec_event_tid = event_wrap.first;
        exec_event = event_wrap.second;
        if (event_wrap.second->erase)
            event_list.erase(exec_event_tid);
        break;
    }
    } /* Unock event_list */
    // dispatch work
    if (has_event_occurs) {
        //std::unique_lock<std::mutex> lock(system_mutex);
        try {
            exec_event->callback();
        } catch (...) {}
    }
    /* Unlock system */
    system_mutex.unlock();
    time_t wait_time = has_event_occurs ? 0 : 500;
    std::this_thread::sleep_for(microseconds(wait_time));
}

void handler() {
    while (state == 1) {
        run();
    }
}

void startEventDispatcher() {
    if (state == 1)
        return;
    system_tid = std::this_thread::get_id();
    owner_tid = system_tid;
    state = 1;
    handler();
}

void endEventDispatcher() {
    state = 0;
}

}
