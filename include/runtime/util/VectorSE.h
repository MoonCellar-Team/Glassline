#pragma once

#include "runtime/midp/Class.h"
#include "runtime/midp/String.h"
#include "runtime/midp/array.h"

namespace midp { template<class T> class array; class Player; class PlayerListener; class String; }
namespace util {

template <class T>
class VectorSE : public midp::Object {
public:
    VectorSE();
    explicit VectorSE(int capacity);
    VectorSE(int capacity,int increment);
    ~VectorSE();
    void setElementAt(T* value,int index);
    void setSize(int size);
    void toArray(midp::array<T*>& array) const;
    void ensureCapacity(int size);
    bool removeElement(T* value);
    bool contains(const T* value) const;
    int indexOf(const T* value) const;
    int size() const;
    T* elementAt(int index) const;
    void addElement(T* obj);
    void removeAllElements();
    T* lastElement() const;
    int capacity() const;
    int indexOf(const T* value, int start) const;
    void removeElementAt(int index);
    T** begin() const;
    T** end() const;
    virtual midp::Class* getClass() const;

    union {
        char m_unk08[0x10];
        struct {
            int m_increment;
            int m_capacity;
            int m_size;
            T** m_data;
        };
    };

    class VectorClass : public midp::Class {
    public:
        VectorClass();
        virtual ~VectorClass();
        virtual midp::String* getName() const;
        virtual midp::Object* newInstance() const;
    };
};

template<class T>
int VectorSE<T>::size() const {
    return m_size;
}

template<class T>
int VectorSE<T>::capacity() const {
    return m_capacity;
}

template<class T>
T* VectorSE<T>::elementAt(int index) const {
    return m_data[index];
}

template<class T>
T** VectorSE<T>::begin() const {
    return m_data;
}

template<class T>
T** VectorSE<T>::end() const {
    return m_data + m_size;
}

template<class T>
T* VectorSE<T>::lastElement() const {
    return elementAt(size() - 1);
}

template<class T>
int VectorSE<T>::indexOf(const T* value, int start) const {
    const int n = size();
    int i = start;
    for (;;) {
        if (n <= i)
            return -1;
        if (elementAt(i) == value)
            return i;
        ++i;
    }
}

template<class T>
int VectorSE<T>::indexOf(const T* value) const {
    return indexOf(value, 0);
}

template<class T>
bool VectorSE<T>::contains(const T* value) const {
    return indexOf(value, 0) != -1;
}

template<class T>
void VectorSE<T>::setElementAt(T* value, int index) {
    T* old = elementAt(index);
    midp::INCREF(value);
    m_data[index] = value;
    midp::DECREF(old);
}

template<class T>
void VectorSE<T>::removeAllElements() {
    if (size() >= 1) {
        int i = 0;
        do {
            setElementAt(NULL, i);
            ++i;
        } while (size() > i);
    }
    m_size = 0;
}

template<class T>
void VectorSE<T>::removeElementAt(int index) {
    int n = size();
    if (n > index + 1) {
        int i = index;
        for (;;) {
            const int src = i + 1;
            setElementAt(elementAt(src), i);
            n = size();
            const bool more = n > i + 2;
            i = src;
            if (!more)
                break;
        }
    }
    setElementAt(NULL, n - 1);
    m_size = m_size - 1;
}

template<class T>
bool VectorSE<T>::removeElement(T* value) {
    const int index = indexOf(value);
    if (index == -1)
        return false;
    removeElementAt(index);
    return true;
}

template<class T>
void VectorSE<T>::ensureCapacity(int requested) {
    if (capacity() >= requested)
        return;
    m_capacity = requested;
    T** old = m_data;
    m_data = static_cast<T**>(::operator new[](static_cast<size_t>(capacity()) * sizeof(T*)));
    if (m_data) {
        if (size() >= 1) {
            int i = 0;
            do {
                m_data[i] = old[i];
                ++i;
            } while (size() > i);
        }
        if (old)
            ::operator delete[](old);
    }
    int i = size();
    if (capacity() <= i)
        return;
    do {
        m_data[i] = NULL;
        ++i;
    } while (capacity() > i);
}

template<class T>
void VectorSE<T>::addElement(T* obj) {
    const int sz = size();
    const int cap = capacity();
    if (sz == cap)
        ensureCapacity(m_increment + cap);
    m_size = m_size + 1;
    setElementAt(obj, size() - 1);
}

template<class T>
void VectorSE<T>::setSize(int newSize) {
    if (size() >= newSize)
        return;
    ensureCapacity(newSize);
    while (size() < newSize)
        addElement(NULL);
}

template<class T>
void VectorSE<T>::toArray(midp::array<T*>& out) const {
    const int n = size();
    const int len = out.m_length;
    const int count = n < len ? n : len;
    if (count < 1)
        return;
    for (int i = 0; i != count; ++i)
        out[i] = m_data[i];
}

template<class T>
VectorSE<T>::VectorSE()
    : m_increment(10), m_capacity(0), m_size(0), m_data(NULL) {
    ensureCapacity(10);
}

template<class T>
VectorSE<T>::VectorSE(int capacity)
    : m_increment(10), m_capacity(0), m_size(0), m_data(NULL) {
    ensureCapacity(capacity);
}

template<class T>
VectorSE<T>::VectorSE(int capacity, int increment)
    : m_increment(increment), m_capacity(0), m_size(0), m_data(NULL) {
    ensureCapacity(capacity);
}

template<class T>
VectorSE<T>::~VectorSE() {
    removeAllElements();
    if (m_data)
        ::operator delete[](m_data);
}

template<class T>
midp::Class* VectorSE<T>::getClass() const {
    return new VectorClass();
}

template<>
void VectorSE<midp::PlayerListener>::setElementAt(midp::PlayerListener* value, int index);
template<>
void VectorSE<midp::Player>::setElementAt(midp::Player* value, int index);
template<>
VectorSE<midp::Player>::VectorSE();
template<>
VectorSE<midp::String>::VectorSE();
template<>
VectorSE<midp::Player>::~VectorSE();
template<>
VectorSE<midp::String>::~VectorSE();
template<>
midp::Class* VectorSE<midp::Player>::getClass() const;
template<>
midp::Class* VectorSE<midp::String>::getClass() const;

template<class T>
midp::Object* VectorSE<T>::VectorClass::newInstance() const {
    return NULL;
}

template<class T>
VectorSE<T>::VectorClass::VectorClass() {}

template<class T>
VectorSE<T>::VectorClass::~VectorClass() {}

template<class T>
midp::String* VectorSE<T>::VectorClass::getName() const {
    return new midp::String("java.util.Vector");
}

}
