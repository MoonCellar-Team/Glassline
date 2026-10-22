#pragma once

#include "runtime/midp/Object.h"

namespace midp {

template <class T>
class array;

class Class;

class InputStream : public Object {
public:
    InputStream();
    virtual ~InputStream();
    virtual Class* getClass() const;
    virtual int available() const;
    virtual void close();
    virtual void mark(int readLimit);
    virtual bool markSupported() const;
    virtual int read() = 0;
    virtual int read(signed char* buf, int n);
    virtual void reset();
    virtual int skip(long n);
    virtual int getPosition();
    virtual bool seek(long offset, int whence);
    virtual int size();
    int read(array<signed char>& buffer, int offset, int length);
    int read(array<signed char>& buffer);
};

unsigned long InputStreamGetBytes(void* info, void* buffer, unsigned long count);
long long InputStreamSkipBytes(void* info, long long count);
void InputStreamRewind(void* info);
void InputStreamRelease(void* info);
void* CGDataProviderCreateWithInputStream(InputStream* stream);

}
