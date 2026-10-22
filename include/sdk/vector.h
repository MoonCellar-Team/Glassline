#pragma once

namespace std {

template <class T>
class vector {
public:
    vector();
    ~vector();
    T* begin();
    T* end();
    unsigned size() const;
    void clear();
    void reserve(unsigned size);
    void resize(unsigned size,const T& value);
    T& operator[](unsigned index);
    bool empty() const;
    void push_back(const T& value);
    T& back();
    void pop_back();
    typedef T* iterator;
    iterator erase(iterator position);
    typedef const T* const_iterator;
    template <class InputIterator>
    void insert(iterator pos, InputIterator first, InputIterator last);
    iterator erase(iterator first, iterator last);

private:
    T* m_begin;
    T* m_end;
    T* m_cap;
};

template<class Iterator> void sort(Iterator first,Iterator last);

template<class T>
const T& min(const T& a, const T& b) {
    return b < a ? b : a;
}
}
