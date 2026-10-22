#include "runtime/midp/SafeReferenceCountedPointer.h"

#include "runtime/midp/Displayable.h"
#ifdef MEDECOMP_PORT
#include "ea_online/Mayhem/Challenge.h"
#include "ea_online/Mayhem/GetStatRequest.h"
#include "ea_online/Mayhem/GetUserListRequest.h"
#include "ea_online/Mayhem/PostStatRequest.h"
#include "ea_online/Mayhem/UserRequest.h"
#endif
#include "runtime/midp/ReferenceCounted.h"

#include <stddef.h>

namespace Mayhem {
class Token;
class UserRequest;
class PostChallenge;
class PostStatRequest;
class GetChallenge;
class GetLeaderboardRequest;
class GetStatRequest;
class GetUserListRequest;
class GetStatListRequest;
}

namespace midp {

template <class T>
static ReferenceCounted* refAt(T* p, int offset) {
    if (p == NULL)
        return NULL;
#ifdef MEDECOMP_PORT
    (void)offset;
    return static_cast<ReferenceCounted*>(p);
#else
    return reinterpret_cast<ReferenceCounted*>(reinterpret_cast<char*>(p) + offset);
#endif
}

template <>
SafeReferenceCountedPointer<Mayhem::UserRequest>::SafeReferenceCountedPointer(Mayhem::UserRequest* p)
    : m_ptr(p) {
    INCREF(refAt(p, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::SafeReferenceCountedPointer(Mayhem::PostStatRequest* p)
    : m_ptr(p) {
    INCREF(refAt(p, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::SafeReferenceCountedPointer(Mayhem::GetStatRequest* p)
    : m_ptr(p) {
    INCREF(refAt(p, 8));
}

template <>
SafeReferenceCountedPointer<Mayhem::UserRequest>::SafeReferenceCountedPointer(
    const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refAt(m_ptr, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::SafeReferenceCountedPointer(
    const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refAt(m_ptr, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::SafeReferenceCountedPointer(
    const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refAt(m_ptr, 8));
}

template <>
SafeReferenceCountedPointer<Mayhem::UserRequest>::~SafeReferenceCountedPointer() {
    DECREF(refAt(m_ptr, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::~SafeReferenceCountedPointer() {
    DECREF(refAt(m_ptr, 8));
}
template <>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::~SafeReferenceCountedPointer() {
    DECREF(refAt(m_ptr, 8));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::SafeReferenceCountedPointer(Mayhem::GetChallenge* p)
    : m_ptr(p) {
    INCREF(refAt(p, 32));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::SafeReferenceCountedPointer(
    const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refAt(m_ptr, 32));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetChallenge>::~SafeReferenceCountedPointer() {
    DECREF(refAt(m_ptr, 32));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::SafeReferenceCountedPointer(
    Mayhem::GetUserListRequest* p)
    : m_ptr(p) {
    INCREF(refAt(p, 16));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::SafeReferenceCountedPointer(
    const SafeReferenceCountedPointer& other)
    : m_ptr(other.m_ptr) {
    INCREF(refAt(m_ptr, 16));
}

template <>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::~SafeReferenceCountedPointer() {
    DECREF(refAt(m_ptr, 16));
}

template <>
SafeReferenceCountedPointer<Displayable>::SafeReferenceCountedPointer(Displayable* p) : m_ptr(p) {
    INCREF(p);
}

template <>
SafeReferenceCountedPointer<Displayable>::~SafeReferenceCountedPointer() {
    DECREF(m_ptr);
}

template <>
SafeReferenceCountedPointer<Mayhem::UserRequest>&
SafeReferenceCountedPointer<Mayhem::UserRequest>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::Token>&
SafeReferenceCountedPointer<Mayhem::Token>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::PostStatRequest>&
SafeReferenceCountedPointer<Mayhem::PostStatRequest>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::PostChallenge>&
SafeReferenceCountedPointer<Mayhem::PostChallenge>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::GetChallenge>&
SafeReferenceCountedPointer<Mayhem::GetChallenge>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::GetStatRequest>&
SafeReferenceCountedPointer<Mayhem::GetStatRequest>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>&
SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>&
SafeReferenceCountedPointer<Mayhem::GetUserListRequest>::operator=(const SafeReferenceCountedPointer& other) {
    SafeReferenceCountedPointer tmp(other);
    swap(tmp);
    return *this;
}

template <>
void SafeReferenceCountedPointer<Displayable>::reset() {
    SafeReferenceCountedPointer tmp;
    swap(tmp);
}

template <>
void SafeReferenceCountedPointer<Displayable>::reset(Displayable* p) {
    SafeReferenceCountedPointer tmp(p);
    swap(tmp);
}

}
