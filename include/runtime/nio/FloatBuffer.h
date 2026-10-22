#pragma once
#include "runtime/nio/Buffer.h"

namespace nio {

class ByteOrder;

class FloatBuffer : public Buffer {
public:
    explicit FloatBuffer(int capacity);
    virtual ~FloatBuffer();
    virtual const ByteOrder* order() const = 0;
    virtual FloatBuffer* put(float value) = 0;
    virtual FloatBuffer* put(int index, float value) = 0;
    virtual FloatBuffer* put(const float* src, int offset, int length) = 0;
    virtual float* lockBuffer() = 0;
    virtual void unlock() = 0;
};

}
