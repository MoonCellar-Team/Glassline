#pragma once

#include <stddef.h>

#include <new>

#include "runtime/midp/Object.h"

namespace midp {

class Class;

class array_data : public ReferenceCounted {
public:
    array_data(void* data, int length, int elementSize, bool owned);
    virtual ~array_data();

    void* m_data;
    int m_length;
    int m_elementSize;
    bool m_owned;
};

class array_base : public Object {
public:
    array_base(int length, array_data* data);
    array_base(const array_base& other);
    virtual ~array_base();
    virtual Class* getClass() const;
    virtual void* discard();

    array_base& operator=(const array_base& other);
    void* data() const;
    void setLengthAndData(int length, array_data* data);

    int m_length;
    array_data* m_data;
};

template <class T>
class array : public array_base {
public:
    array();
    explicit array(int length);
    array(const T* data, int length);
    array(const array& other);
    virtual ~array();
    virtual void* discard();
    array& operator=(const array& other);
    bool operator==(const T* p) const;
    bool operator!=(const T* p) const;
    T& operator[](int i) { return static_cast<T*>(data())[i]; }
    const T& operator[](int i) const { return static_cast<const T*>(data())[i]; }
    T* begin();

    bool m_flag;
};

template<class T>
array<T>& array<T>::operator=(const array& other) {
    array_base::operator=(other);
    m_flag = other.m_flag;
    return *this;
}

template<class T>
bool array<T>::operator==(const T* p) const {
    return data() == p;
}

template<class T>
void* array<T>::discard() {
    int count = m_length;
    void* p = array_base::discard();
    if (p != NULL) {
        T* it = static_cast<T*>(p);
        for (int i = 0; i < count; i++)
            it[i].~T();
        ::operator delete(p);
    }
    return NULL;
}

template<class T>
array<T>::~array() {
    discard();
}

template<class T>
array<T>::array() : array_base(0, NULL) {}

template<class T>
array<T>::array(const array& other) : array_base(other) {}

template<class T>
array<T>::array(int length) : array_base(0, NULL) {
    if (length < 1)
        return;
    T* buf = static_cast<T*>(::operator new((size_t)length * sizeof(T)));
    for (int i = 0; i < length; i++)
        new (buf + i) T();
    void* raw = ::operator new(sizeof(array_data));
    setLengthAndData(length, new (raw) array_data(buf, sizeof(T), length, true));
}

template<class T>
bool array<T>::operator!=(const T* p) const {
    return data() != p;
}

template<class T>
T* array<T>::begin() {
    return static_cast<T*>(data());
}

template<>
array<int>::array(const int* data, int length);

template <class T>
array<array<T>> array2D(int rows, int cols);

}
