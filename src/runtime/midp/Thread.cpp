#include "runtime/midp/Thread.h"

#include <stddef.h>

#include "runtime/midp/PThreadRuntimeObject.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ThreadClass.h"

extern "C" int usleep(unsigned int usec);

namespace midp {

Thread::Thread() : m_target(NULL), m_alive(false), m_native(NULL) {}

void Thread::run() {
    if (m_target != NULL)
        m_target->run();
}

bool Thread::isAlive() const {
    return m_alive;
}

void Thread::sleep(long millis) {
    usleep((unsigned int)(millis * 1000));
}

extern "C" void medecomp_on_yield();

void Thread::yield() {
    medecomp_on_yield();
    sleep(1);
}

void Thread::start() {
    m_alive = true;
    m_native = new PThreadRuntimeObject();
    m_native->startDetached(Thread_posixRun, this);
}

Thread::~Thread() {
    PThreadRuntimeObject* native = m_native;
    m_native = NULL;
    if (native != NULL)
        native->~PThreadRuntimeObject();
    DECREF(m_target);
}

Thread::Thread(Runnable* target) : m_target(target), m_alive(false), m_native(NULL) {
    INCREF(m_target);
}

Class* Thread::getClass() const {
    return new ThreadClass();
}

void Thread::setDead() {
    m_alive = false;
}

void Thread::join() {
    while (isAlive())
        yield();
}

}
