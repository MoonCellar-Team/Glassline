#pragma once

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"

namespace Mayhem {
class Token;
class UserRequest;
class PostChallenge;
class PostStatRequest;
class GetChallenge;
class GetLeaderboardRequest;
class GetStatRequest;
class GetUserListRequest;
}

namespace midp {

class Displayable;

template <class T>
class SafeReferenceCountedPointer {
public:
    SafeReferenceCountedPointer();
    SafeReferenceCountedPointer(T* p);
    SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
    ~SafeReferenceCountedPointer();
    SafeReferenceCountedPointer& operator=(const SafeReferenceCountedPointer& other);
    T* operator->();
    T* operator->() const;
    operator T*() const;
    bool operator==(T* p) const;
    bool operator!=(T* p) const;
    T* unsafeGetWrappedPointer() const;
    void swap(SafeReferenceCountedPointer& other);
    void reset();
    void reset(T* p);

    T* m_ptr;
};

template <class T>
SafeReferenceCountedPointer<T>::SafeReferenceCountedPointer() : m_ptr(0) {}

template <class T>
SafeReferenceCountedPointer<T>::SafeReferenceCountedPointer(T* p) : m_ptr(p) {
    INCREF(reinterpret_cast<ReferenceCounted*>(p));
}

template <class T>
SafeReferenceCountedPointer<T>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <class T>
SafeReferenceCountedPointer<T>::~SafeReferenceCountedPointer() {
    DECREF(reinterpret_cast<ReferenceCounted*>(m_ptr));
}

template <class T>
T* SafeReferenceCountedPointer<T>::operator->() const {
    return m_ptr;
}

template <class T>
T* SafeReferenceCountedPointer<T>::operator->() {
    return m_ptr;
}

template <class T>
SafeReferenceCountedPointer<T>::operator T*() const {
    return m_ptr;
}

template <class T>
T* SafeReferenceCountedPointer<T>::unsafeGetWrappedPointer() const {
    return m_ptr;
}

template <class T>
bool SafeReferenceCountedPointer<T>::operator==(T* p) const {
    return m_ptr == p;
}

template <class T>
bool SafeReferenceCountedPointer<T>::operator!=(T* p) const {
    return m_ptr != p;
}

template <class T>
void SafeReferenceCountedPointer<T>::swap(SafeReferenceCountedPointer& other) {
    T* tmp = m_ptr;
    m_ptr = other.m_ptr;
    other.m_ptr = tmp;
}

template<>
SafeReferenceCountedPointer<Mayhem::UserRequest>::SafeReferenceCountedPointer(Mayhem::UserRequest* p);
template<>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::SafeReferenceCountedPointer(Mayhem::PostStatRequest* p);
template<>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::SafeReferenceCountedPointer(Mayhem::GetStatRequest* p);
template<>
SafeReferenceCountedPointer<Mayhem::UserRequest>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::UserRequest>::~SafeReferenceCountedPointer();
template<>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::~SafeReferenceCountedPointer();
template<>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::~SafeReferenceCountedPointer();
template<>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::SafeReferenceCountedPointer(Mayhem::GetChallenge* p);
template<>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::~SafeReferenceCountedPointer();
template<>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::SafeReferenceCountedPointer(Mayhem::GetUserListRequest* p);
template<>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::SafeReferenceCountedPointer(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::~SafeReferenceCountedPointer();
template<>
SafeReferenceCountedPointer<Displayable>::SafeReferenceCountedPointer(Displayable* p);
template<>
SafeReferenceCountedPointer<Displayable>::~SafeReferenceCountedPointer();
template<>
void SafeReferenceCountedPointer<Displayable>::reset();
template<>
void SafeReferenceCountedPointer<Displayable>::reset(Displayable* p);

template<>
SafeReferenceCountedPointer<Mayhem::UserRequest>&
SafeReferenceCountedPointer<Mayhem::UserRequest>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::Token>&
SafeReferenceCountedPointer<Mayhem::Token>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>&
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::PostChallenge>&
SafeReferenceCountedPointer<Mayhem::PostChallenge>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetChallenge>&
SafeReferenceCountedPointer<Mayhem::GetChallenge>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>&
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>&
SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>::operator=(const SafeReferenceCountedPointer& other);
template<>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>&
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::operator=(const SafeReferenceCountedPointer& other);

}
