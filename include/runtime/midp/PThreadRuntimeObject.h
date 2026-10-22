#pragma once

#include "sdk/pthread.h"

namespace midp {

class Thread;

class PThreadRuntimeObject {
public:
    PThreadRuntimeObject();
    virtual ~PThreadRuntimeObject();
    void startDetached(void* (*start)(void*), Thread* thread);

    pthread_t m_thread;
};

}
