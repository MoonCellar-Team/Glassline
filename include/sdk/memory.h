#pragma once

namespace std {

template<class T>
class auto_ptr {
public:
    explicit auto_ptr(T* p = 0) : m_ptr(p) {}
    auto_ptr(auto_ptr& other) : m_ptr(other.release()) {}
    ~auto_ptr() { delete m_ptr; }

    T* operator->() const { return m_ptr; }
    T* get() const { return m_ptr; }
    T* release() {
        T* p = m_ptr;
        m_ptr = 0;
        return p;
    }

    T* m_ptr;
};

}
