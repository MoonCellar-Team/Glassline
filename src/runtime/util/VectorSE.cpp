#include "runtime/util/VectorSE.h"

#include <stddef.h>

#include "runtime/midp/Player.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/String.h"
#include "runtime/midp/array.h"

void* operator new(__SIZE_TYPE__);
void operator delete(void*) noexcept;
void* operator new[](__SIZE_TYPE__);
void operator delete[](void*) noexcept;

namespace {

template <class T>
midp::ReferenceCounted* refBase(T* obj) {
    return obj;
}

template<class T>
void setElementAtAdjusted(util::VectorSE<T>* self, T* value, int index) {
    T* old = self->elementAt(index);
    midp::INCREF(refBase(value));
    self->m_data[index] = value;
    midp::DECREF(refBase(old));
}

}

namespace util {

template<>
void VectorSE<midp::PlayerListener>::setElementAt(midp::PlayerListener* value, int index) {
    setElementAtAdjusted(this, value, index);
}

template<>
void VectorSE<midp::Player>::setElementAt(midp::Player* value, int index) {
    setElementAtAdjusted(this, value, index);
}

template<>
VectorSE<midp::Player>::VectorSE()
    : m_increment(10), m_capacity(0), m_size(0), m_data(NULL) {
    ensureCapacity(10);
}

template<>
VectorSE<midp::String>::VectorSE()
    : m_increment(10), m_capacity(0), m_size(0), m_data(NULL) {
    ensureCapacity(10);
}

template<>
VectorSE<midp::Player>::~VectorSE() {
    removeAllElements();
    if (m_data)
        ::operator delete[](m_data);
}

template<>
VectorSE<midp::String>::~VectorSE() {
    removeAllElements();
    if (m_data)
        ::operator delete[](m_data);
}

template<>
midp::Class* VectorSE<midp::Player>::getClass() const {
    return new VectorClass();
}

template<>
midp::Class* VectorSE<midp::String>::getClass() const {
    return new VectorClass();
}

}
