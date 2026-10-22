#include "runtime/midp/ReferenceCounted.h"

#include <libkern/OSAtomic.h>

namespace midp {

ReferenceCounted::ReferenceCounted() : m_refCount(0) {}

ReferenceCounted::~ReferenceCounted() {
    m_refCount = -1;
}

bool ReferenceCounted::_decRef() {
    bool last = m_refCount == 1;
    OSAtomicDecrement32(&m_refCount);
    return last;
}

void ReferenceCounted::_incRef() {
    OSAtomicIncrement32(&m_refCount);
}

int ReferenceCounted::getRefCount() const {
    return m_refCount;
}

}
