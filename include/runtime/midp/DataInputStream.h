#pragma once

#include "runtime/midp/InputStream.h"

namespace midp {

class Class;
class String;

class DataInputStream : public InputStream {
public:
    explicit DataInputStream(InputStream* in);
    virtual ~DataInputStream();
    virtual Class* getClass() const;
    virtual int available() const;
    void close();
    int readInt();
    signed char readByte();
    short readShort();
    float readFloat();
    int readUnsignedShort();
    bool readBoolean();
    int readUnsignedByte();
    void readFully(array<signed char>& bytes);
    void readFully(signed char* bytes, int length);
    void readFully(array<unsigned char>& bytes);
    String* readUTF();
    bool eofExceptionThrown() const;
    int read(signed char* bytes, int length);
    virtual int read();
    void mark(int limit);
    virtual bool markSupported() const;
    virtual void reset();
    virtual int skip(long length);
    virtual int getPosition();
    virtual bool seek(long offset, int whence);
    virtual int size();
    void readFully(array<signed char>& bytes, int offset, int length);
    void readFully(array<unsigned char>& bytes, int offset, int length);
    bool eofExceptionThrown();

    InputStream* m_in;
    bool m_eof;
};

}
