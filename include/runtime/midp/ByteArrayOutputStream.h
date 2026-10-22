#pragma once

#include "runtime/midp/OutputStream.h"
#include "runtime/midp/array.h"

namespace midp {

class Class;

class ByteArrayOutputStream : public OutputStream {
public:
    ByteArrayOutputStream();
    virtual ~ByteArrayOutputStream();
    virtual Class* getClass() const;
    virtual void write(unsigned char b);
    virtual void write(const void* data, int length);
    virtual bool close();
    int size();
    void writeTo(OutputStream* out);
    void reset();
    void ensureCapacity(int minCapacity);

    array<signed char> m_buf;
    int m_count;
};

}
