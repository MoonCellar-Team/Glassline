#pragma once

#include "runtime/midp/InputStream.h"
#include "sdk/stdio.h"

namespace midp {

class String;

class FileInputStreamIPhone : public InputStream {
public:
    explicit FileInputStreamIPhone(String* path);
    virtual ~FileInputStreamIPhone();
    virtual int read();
    bool loadSuccessful();
    virtual int available() const;
    virtual void close();
    virtual void mark(int readLimit);
    virtual bool markSupported() const;
    virtual int read(signed char* buf, int n);
    virtual void reset();
    virtual int getPosition();
    virtual bool seek(long offset, int whence);
    virtual int size();

    FILE* m_file;
    long m_mark;
    int m_size;
    bool m_eof;
};

}
