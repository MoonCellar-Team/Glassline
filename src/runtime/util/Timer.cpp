#include "runtime/util/Timer.h"

#include <stddef.h>

#include "runtime/midp/Runtime.h"
#include "runtime/util/TimerClass.h"

namespace util {

class TimerTask {
public:
    void cancel();
};

static midp::ReferenceCounted* timerTaskRef(TimerTask* task) {
    if (task == NULL)
        return NULL;
    const char* vptr = *reinterpret_cast<const char* const*>(task);
    int offset = *reinterpret_cast<const int*>(vptr - 12);
    return reinterpret_cast<midp::ReferenceCounted*>(reinterpret_cast<char*>(task) + offset);
}

midp::Class* Timer::getClass() const {
    return new TimerClass();
}

void Timer::setTask(TimerTask* task) {
    midp::INCREF(timerTaskRef(task));
    midp::DECREF(timerTaskRef(m_task));
    m_task = task;
}

TimerTask* Timer::getTask() const {
    return m_task;
}

void Timer::schedule(TimerTask* task, long delay, long period) {
    setTask(task);
    m_delay = delay;
    m_period = period;
    m_unk14 = false;
    midp::Runtime::getRuntime()->scheduleTimer(this, delay, period);
}

void Timer::cancel() {
    TimerTask* task = m_task;
    if (task != NULL) {
        midp::INCREF(timerTaskRef(task));
        setTask(NULL);
        task->cancel();
        midp::DECREF(timerTaskRef(task));
        return;
    }
    midp::INCREF(NULL);
    setTask(NULL);
}

Timer::~Timer() {
    midp::DECREF(timerTaskRef(m_task));
    m_task = NULL;
}

Timer::Timer() : m_task(NULL), m_delay(0), m_period(0), m_unk14(false) {}

}
