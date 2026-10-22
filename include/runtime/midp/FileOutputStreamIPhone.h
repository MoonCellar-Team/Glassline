#pragma once

#include "runtime/midp/OutputStream.h"
#include "sdk/stdio.h"

namespace midp {

class String;

class FileOutputStreamIPhone : public OutputStream {
public:
    explicit FileOutputStreamIPhone(String* name);
    virtual ~FileOutputStreamIPhone();
    virtual void write(unsigned char b);
    virtual void write(const void* data, int length);
    virtual bool close();
    virtual long getPosition();
    virtual bool seek(long offset, int whence);
    bool loadSuccessful();

    FILE* m_file;
};

}
