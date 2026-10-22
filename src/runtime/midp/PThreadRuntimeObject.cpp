#include "runtime/midp/PThreadRuntimeObject.h"

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/Thread.h"

namespace midp {

PThreadRuntimeObject::PThreadRuntimeObject() : m_thread(NULL) {}

PThreadRuntimeObject::~PThreadRuntimeObject() {}

void PThreadRuntimeObject::startDetached(void* (*start)(void*), Thread* thread) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, 2);
    ReferenceCounted* ref = thread;
    INCREF(ref);
    pthread_create(&m_thread, &attr, start, thread);
    pthread_attr_destroy(&attr);
}

}
