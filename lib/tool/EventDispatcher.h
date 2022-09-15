#ifndef _EVENTDISPATCHER_H_
#define _EVENTDISPATCHER_H_

#include <functional>
#include <cstddef>
#include <cstdint>
#include <ctime>

namespace EventDispatcher {

using timeoutID           = int64_t;
using timeoutCallback     = std::function<void(void)>;
using conditionalCallback = std::function<bool(void)>;

timeoutID setTimeout(timeoutCallback callback, time_t delay = 0);
timeoutID setInterval(timeoutCallback callback, time_t delay = 0);
timeoutID setConditional(timeoutCallback callback, conditionalCallback cond);
void clearTimeout(timeoutID tid);
void clearInterval(timeoutID tid);

void setTimeoutSync(timeoutCallback callback, time_t delay);
void await(conditionalCallback callback);

class SystemLock {
    friend SystemLock getSystemLock();
private:
    bool locked;
    SystemLock();
public:
    ~SystemLock();
    void lock();
    void unlock();
};

SystemLock getSystemLock();

size_t todoCount();

void startEventDispatcher();
void endEventDispatcher();

template<class RTy>
class Promise {
public:

    using Resolve = std::function<void(RTy)>;
    using Callback = std::function<void(Resolve)>;

private:

    enum State { PENDING, FULFILLED } state;
    Resolve onFulfilled;

    RTy data;

public:

    Promise(Callback callback) : onFulfilled(nullptr) {
        state = PENDING;
        callback([this](RTy data) {
            this->state = FULFILLED;
            this->data = data;
            if (onFulfilled != nullptr)
                this->onFulfilled(data);
        });
    }

    void then(Resolve callback) {
        if (onFulfilled != nullptr) return;
        onFulfilled = callback;
        if (state == FULFILLED)
            onFulfilled(data);
    }

};

}

#endif