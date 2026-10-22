#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class String;
class Class;

template <class T>
class array;

class OutputStream : public Object {
public:
    OutputStream();
    virtual ~OutputStream();
    virtual Class* getClass() const;
    virtual void write(unsigned char b) = 0;
    virtual void write(array<signed char>& buffer);
    virtual void write(array<signed char>& buffer, int offset, int length);
    virtual void write(const void* data, int length);
    virtual bool close();
    static OutputStream* getResourceAsStream(String* name);
};

}
