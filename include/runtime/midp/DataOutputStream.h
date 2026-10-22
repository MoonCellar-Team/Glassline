#pragma once

#include "runtime/midp/OutputStream.h"

namespace midp {

class Class;
class String;

class DataOutputStream : public OutputStream {
public:
    void writeBoolean(bool v);
    void writeByte(unsigned char v);
    void writeShort(short v);
    void writeInt(int v);
    explicit DataOutputStream(OutputStream* out);
    virtual ~DataOutputStream();
    void writeUnsignedShort(unsigned short v);
    virtual void write(const void* bytes, int length);
    virtual void write(unsigned char v);
    void writeFloat(float v);
    virtual bool close();
    virtual void write(array<signed char>& buffer);
    virtual void write(array<signed char>& buffer, int offset, int length);
    void writeUTF(String* str);
    virtual Class* getClass() const;

    OutputStream* m_out;
};

}
