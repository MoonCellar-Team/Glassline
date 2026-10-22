#pragma once

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"

namespace midp {

template <class T>
class ReferenceCountedPointer {
public:
    ReferenceCountedPointer() : m_ptr(0) {}
    ReferenceCountedPointer(T* p) : m_ptr(p) { if (m_ptr) INCREF(m_ptr); }
    ReferenceCountedPointer(const ReferenceCountedPointer& o) : m_ptr(o.m_ptr) { if (m_ptr) INCREF(m_ptr); }
    ~ReferenceCountedPointer() { if (m_ptr) DECREF(m_ptr); }

    template <class U>
    ReferenceCountedPointer& operator=(U* p) {
        if (p) INCREF(p);
        if (m_ptr) DECREF(m_ptr);
        m_ptr = p;
        return *this;
    }
    ReferenceCountedPointer& operator=(const ReferenceCountedPointer& o) { return operator=(o.m_ptr); }

    operator T*() const { return m_ptr; }
    T* operator->() { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() { return *m_ptr; }
    bool operator==(T* p) const { return m_ptr == p; }
    bool operator!=(T* p) const { return m_ptr != p; }

    template <class U>
    ReferenceCountedPointer(U* p) : m_ptr(reinterpret_cast<T*>(p)) {
        if (m_ptr)
            INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
    }

    T* m_ptr;
};

}
