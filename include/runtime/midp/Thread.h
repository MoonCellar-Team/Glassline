#pragma once

#include "runtime/midp/Runnable.h"

namespace midp {

class Class;
class PThreadRuntimeObject;

class Thread : public Runnable {
public:
    static void yield();
    explicit Thread(Runnable* target);
    Thread();
    virtual ~Thread();
    void join();
    virtual void run();
    virtual Class* getClass() const;
    void start();
    static void sleep(long millis);
    void setDead();
    bool isAlive() const;

    Runnable* m_target;
    bool m_alive;
    PThreadRuntimeObject* m_native;
};

void* Thread_posixRun(void* arg);
void* Thread_WebImageRun(void* arg);

}
