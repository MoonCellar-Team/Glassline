#pragma once
#include <stddef.h>
#include "m3g/Placement.h"
namespace m3g {
template<class T,unsigned Size> class OpaqueObject {
public:
    template<class... Args> explicit OpaqueObject(Args... args) { new (m_storage) T(args...); }
    ~OpaqueObject() { get()->~T(); }
    T* get() { return reinterpret_cast<T*>(m_storage); }
    const T* get() const { return reinterpret_cast<const T*>(m_storage); }
private:
    static_assert(sizeof(void*) != 4 || sizeof(T) <= Size, "OpaqueObject: объект больше места из бинарника");
    alignas(T) unsigned char m_storage[Size < sizeof(T) ? sizeof(T) : Size];
};
}
