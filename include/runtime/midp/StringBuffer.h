#pragma once

#include "runtime/midp/Object.h"

struct __CFString;

namespace midp {

class Class;
class String;

class StringBuffer : public Object {
public:
    StringBuffer();
    virtual ~StringBuffer();
    int length() const;
    StringBuffer* append(const char* value);
    StringBuffer* append(unsigned value);
    StringBuffer* append(int value);
    void copyCString(char* output,unsigned capacity) const;
    void setLength(int length);
    StringBuffer* append(char c);
    String* toString() const;
    explicit StringBuffer(int capacity);
    StringBuffer* append(String* value);
    StringBuffer* append(unsigned short c);
    virtual Class* getClass() const;

    struct __CFString* m_string;
};

}
