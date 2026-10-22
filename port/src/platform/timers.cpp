#include <algorithm>
#include <chrono>
#include <mutex>
#include <vector>

#include "port/objc.h"
#include "port/platform.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/Runtime.h"
#include "runtime/util/Timer.h"
#include "runtime/util/TimerTask.h"
#include "sdk/Foundation.h"

class CocoaTimer : public NSObject {
public:
    static CocoaTimer* alloc();
    CocoaTimer* init();
    void schedule_withDelay_andPeriod_(util::Timer* timer, long delay, long period);
    void scheduleOnce_withDelay_(util::Timer* timer, long delay);
};

namespace {

using Clock = std::chrono::steady_clock;

struct TimerObject : port::objc::Object {
    util::Timer* timer = nullptr;
    bool once = false;
    Clock::duration interval = std::chrono::microseconds(16667);
    Clock::time_point next;
    bool scheduled = false;
    TimerObject();
};

const port::objc::Class kCocoaTimer = {
    {"CocoaTimer", port::objc::destroyAs<TimerObject>, nullptr}, &port::objc::NSObjectClass, {}};

TimerObject::TimerObject() : port::objc::Object(&kCocoaTimer) {}

std::mutex s_mutex;
std::vector<TimerObject*> s_scheduled;

void setTimer(TimerObject* t, util::Timer* next) {
    if (t->timer) {
        midp::Runtime::getRuntime()->removeTimer(t->timer);
        midp::DECREF(t->timer);
    }
    t->timer = next;
    if (next) midp::INCREF(next);
}

void schedule(TimerObject* t, util::Timer* timer, long ms, bool once) {
    setTimer(t, timer);
    t->once = once;
    t->interval = std::chrono::milliseconds(ms);
    t->next = Clock::now() + t->interval;
    std::lock_guard<std::mutex> l(s_mutex);
    if (!t->scheduled) {
        t->scheduled = true;
        CFRetain(t);
        s_scheduled.push_back(t);
    }
}

void invalidate(TimerObject* t) {
    {
        std::lock_guard<std::mutex> l(s_mutex);
        if (!t->scheduled) return;
        t->scheduled = false;
        s_scheduled.erase(std::remove(s_scheduled.begin(), s_scheduled.end(), t), s_scheduled.end());
    }
    CFRelease(t);
}

void run(TimerObject* t) {
    if (!t->timer) return;
    util::TimerTask* task = t->timer->getTask();
    if (!task) return;
    if (!task->wantsToCancel()) {
        NSAutoreleasePool* pool = NSAutoreleasePool::alloc()->init();
        task->run();
        pool->release();
        if (t->once) t->timer->cancel();
        if (!task->wantsToCancel()) return;
    }
    setTimer(t, nullptr);
    invalidate(t);
}

}

CocoaTimer* CocoaTimer::alloc() { return reinterpret_cast<CocoaTimer*>(new TimerObject()); }

CocoaTimer* CocoaTimer::init() { return this; }

void CocoaTimer::schedule_withDelay_andPeriod_(util::Timer* timer, long, long period) {
    schedule(reinterpret_cast<TimerObject*>(this), timer, period, false);
}

void CocoaTimer::scheduleOnce_withDelay_(util::Timer* timer, long delay) {
    schedule(reinterpret_cast<TimerObject*>(this), timer, delay, true);
}

namespace port {
namespace platform {

void runTimers() {
    Clock::time_point now = Clock::now();
    std::vector<TimerObject*> due;
    {
        std::lock_guard<std::mutex> l(s_mutex);
        for (TimerObject* t : s_scheduled)
            if (t->next <= now) {
                CFRetain(t);
                due.push_back(t);
            }
    }
    for (TimerObject* t : due) {
        if (t->scheduled) {
            if (t->once) {
                invalidate(t);
            } else {
                t->next += t->interval;
                if (t->next <= now) t->next = now + t->interval;
            }
            run(t);
        }
        CFRelease(t);
    }
}

int msUntilNextTimer() {
    std::lock_guard<std::mutex> l(s_mutex);
    if (s_scheduled.empty()) return -1;
    Clock::time_point first = s_scheduled[0]->next;
    for (TimerObject* t : s_scheduled) first = std::min(first, t->next);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(first - Clock::now()).count();
    return ms < 0 ? 0 : static_cast<int>(ms);
}

}
}
