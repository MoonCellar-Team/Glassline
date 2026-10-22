#pragma once

#include "runtime/midp/InputStream.h"
#include "runtime/midp/array.h"

namespace midp {

class Class;

class ByteArrayInputStream : public InputStream {
public:
    explicit ByteArrayInputStream(array<signed char>& bytes);
    virtual ~ByteArrayInputStream();
    virtual Class* getClass() const;
    virtual int available() const;
    virtual void mark(int readLimit);
    virtual bool markSupported() const;
    virtual void reset();
    virtual int skip(long n);
    virtual int getPosition();
    virtual int size();
    virtual bool seek(long offset, int whence);
    virtual int read();
    virtual int read(signed char* buf, int n);

    array<signed char> m_buf;
    int m_off;
    int m_count;
    int m_mark;
    int m_pos;
};

}
