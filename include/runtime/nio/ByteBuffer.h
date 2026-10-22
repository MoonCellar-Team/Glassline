#pragma once
#include "runtime/nio/Buffer.h"
#include "runtime/nio/ByteOrder.h"

namespace nio {

class FloatBuffer;

class ByteBuffer : public Buffer {
public:
    explicit ByteBuffer(int capacity);
    virtual ~ByteBuffer();

    static ByteBuffer* allocateDirect(int bytes);
    const ByteOrder* order() const;
    ByteBuffer* order(const ByteOrder* value);

    virtual FloatBuffer* asFloatBuffer() = 0;
    virtual ByteBuffer* put(const signed char* src, int length) = 0;
    virtual ByteBuffer* putInt(int value) = 0;
    virtual ByteBuffer* get(signed char* dst, int length) = 0;
    virtual int getInt() = 0;

    const ByteOrder* m_order;
};

}
