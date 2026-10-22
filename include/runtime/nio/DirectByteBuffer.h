#pragma once
#include "runtime/nio/ByteBuffer.h"

namespace nio {

class DirectByteBuffer : public ByteBuffer {
public:
    explicit DirectByteBuffer(int capacity);
    virtual ~DirectByteBuffer();
    virtual midp::Class* getClass() const;
    virtual bool isReadOnly() const;
    virtual FloatBuffer* asFloatBuffer();
    virtual ByteBuffer* put(const signed char* src, int length);
    virtual ByteBuffer* putInt(int value);
    virtual ByteBuffer* get(signed char* dst, int length);
    virtual int getInt();

    signed char* m_data;
};

}
