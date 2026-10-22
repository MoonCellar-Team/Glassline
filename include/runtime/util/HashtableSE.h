#pragma once
#include "runtime/midp/Class.h"
#include "runtime/midp/Object.h"
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"
#include "runtime/midp/ReferenceCountedHardReference.h"

namespace util {

template<class K, class V>
class HashtableSE : public midp::Object {
public:
    explicit HashtableSE(int capacity);
    ~HashtableSE();
    V* get(K* key) const;
    bool containsKey(K* key) const;
    V* put(K* key, V* value);
    int capacity() const;
    int size() const;
    void ensureCapacity(int requested);
    void clearHashMap();
    void rehash();
    virtual midp::Class* getClass() const;

    VectorSE<K> m_keys;
    VectorSE<V> m_values;
    midp::array<int> m_index;

    int getStartPosition(int hash, int cap) const;
    int getEndPosition(int start, int cap) const;
    bool isKeyAt(const K* key, int hash, int index) const;
    int linearProbeForKey(const K* key) const;
    int linearProbeForKeyOrEmptyOrRemoved(const K* key) const;

    class HashtableClass : public midp::Class {
    public:
        HashtableClass();
        virtual ~HashtableClass();
        virtual midp::String* getName() const;
        virtual midp::Object* newInstance() const;
    };
};

template<class K, class V>
midp::Object* HashtableSE<K, V>::HashtableClass::newInstance() const {
    return NULL;
}

template<class K, class V>
HashtableSE<K, V>::HashtableClass::HashtableClass() {}

template<class K, class V>
HashtableSE<K, V>::HashtableClass::~HashtableClass() {}

template<class K, class V>
midp::String* HashtableSE<K, V>::HashtableClass::getName() const {
    return new midp::String("java.util.Hashtable");
}

template <class K, class V>
int HashtableSE<K, V>::capacity() const {
    return m_index.m_length;
}

template <class K, class V>
int HashtableSE<K, V>::size() const {
    return m_keys.size();
}

template <class K, class V>
int HashtableSE<K, V>::getStartPosition(int hash, int cap) const {
    return (hash & 0x7fffffff) % cap;
}

template <class K, class V>
int HashtableSE<K, V>::getEndPosition(int start, int cap) const {
    if (start == 0)
        start = cap;
    return start - 1;
}

template <class K, class V>
bool HashtableSE<K, V>::isKeyAt(const K* key, int hash, int index) const {
    K* element = m_keys.elementAt(index);
    if (element->hashCode() != hash)
        return false;
    if (element == key)
        return true;
    return key->equals(element);
}

template <class K, class V>
int HashtableSE<K, V>::linearProbeForKey(const K* key) const {
    const int hash = key->hashCode();
    const int cap = capacity();
    int index = getStartPosition(hash, cap);
    const int end = getEndPosition(index, cap);
    for (;;) {
        if (index == cap)
            index = 0;
        const int slot = m_index[index];
        if (slot < 0) {
            if (slot == -1)
                return -1;
        } else if (isKeyAt(key, hash, slot)) {
            return index;
        }
        if (index == end)
            return -1;
        ++index;
    }
}

template <class K, class V>
V* HashtableSE<K, V>::get(K* key) const {
    midp::ReferenceCountedHardReference hold(key);
    const int place = linearProbeForKey(key);
    V* result = NULL;
    if (place >= 0)
        result = m_values.elementAt(m_index[place]);
    return result;
}

template <class K, class V>
int HashtableSE<K, V>::linearProbeForKeyOrEmptyOrRemoved(const K* key) const {
    const int hash = key->hashCode();
    const int cap = capacity();
    int index = getStartPosition(hash, cap);
    const int end = getEndPosition(index, cap);
    for (;;) {
        if (index == cap)
            index = 0;
        const int slot = m_index[index];
        if (slot < 0) {
            if (slot > static_cast<int>(0xfffffffd))
                return index;
        } else if (isKeyAt(key, hash, slot)) {
            return index;
        }
        if (index == end)
            return -1;
        ++index;
    }
}

template <class K, class V>
bool HashtableSE<K, V>::containsKey(K* key) const {
    return linearProbeForKey(key) >= 0;
}

template <class K, class V>
void HashtableSE<K, V>::clearHashMap() {
    const int n = capacity();
    for (int i = 0; i < n; ++i)
        m_index[i] = -1;
}

template <class K, class V>
void HashtableSE<K, V>::rehash() {
    clearHashMap();
    const int n = size();
    if (n < 1)
        return;
    for (int i = 0; i < n; ++i) {
        K* key = m_keys.elementAt(i);
        const int place = linearProbeForKeyOrEmptyOrRemoved(key);
        m_index[place] = i;
    }
}

template <class K, class V>
HashtableSE<K, V>::~HashtableSE() {
    m_keys.removeAllElements();
    m_values.removeAllElements();
}

template <class K, class V>
midp::Class* HashtableSE<K, V>::getClass() const {
    return new HashtableClass();
}

template <class K, class V>
void HashtableSE<K, V>::ensureCapacity(int requested) {
    if (capacity() >= requested)
        return;
    midp::array<int> grown(requested);
    m_index = grown;
    rehash();
}

template <class K, class V>
V* HashtableSE<K, V>::put(K* key, V* value) {
    const int found = linearProbeForKey(key);
    if (found >= 0) {
        m_values.setElementAt(value, m_index[found]);
        return value;
    }
    const int used = size() * 100;
    const int cap = capacity();
    if (used / cap >= 75)
        ensureCapacity((cap << 1) | 1);
    const int place = linearProbeForKeyOrEmptyOrRemoved(key);
    m_index[place] = size();
    m_keys.addElement(key);
    m_values.addElement(value);
    return value;
}

template <class K, class V>
HashtableSE<K, V>::HashtableSE(int cap)
    : m_keys(cap), m_values(cap), m_index() {
    ensureCapacity(cap);
}

}
