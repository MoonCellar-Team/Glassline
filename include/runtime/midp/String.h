#pragma once

#include "runtime/midp/Object.h"
#include "runtime/midp/array.h"

struct __CFString;

namespace midp {

template <class T>
class array;

class Class;

class String : public Object {
public:
    String();
    explicit String(const char* s);
    virtual bool equals(const Object* other) const;
    const __CFString* getCFString() const;
    static String* valueOf(int value);
    bool equals(const char* s) const;
    String* toUpperCase() const;
    explicit String(const __CFString* s);
    int length() const;
    String* substring(int begin, int end) const;
    bool startsWith(String* prefix) const;
    unsigned short charAt(int index) const;
    bool equals(String* other) const;
    String(const array<signed char>& bytes, const char* encoding);
    virtual ~String();
    virtual Class* getClass() const;
    virtual int hashCode() const;
    bool equals(const __CFString* s) const;
    explicit String(__CFString* s);
    String(String* other);
    bool startsWith(String* prefix, int offset) const;
    int indexOf(String* str, int from) const;
    int lastIndexOf(unsigned short ch) const;
    int lastIndexOf(unsigned short ch, int from) const;
    array<signed char> getBytes(const char* encoding);
    void set(const __CFString* s);

    const __CFString* m_string;
    mutable bool m_hashValid;
    mutable int m_hash;
};

}
