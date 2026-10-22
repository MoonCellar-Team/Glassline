#pragma once
#include "runtime/nio/DirectByteBuffer.h"
#include "runtime/nio/FloatBuffer.h"

namespace nio {

class NativeDirectFloatBuffer : public FloatBuffer {
public:
    explicit NativeDirectFloatBuffer(DirectByteBuffer* buffer);
    virtual ~NativeDirectFloatBuffer();
    virtual bool isReadOnly() const;
    virtual const ByteOrder* order() const;
    virtual FloatBuffer* put(float value);
    virtual FloatBuffer* put(int index, float value);
    virtual FloatBuffer* put(const float* src, int offset, int length);
    virtual float* lockBuffer();
    virtual void unlock();

    DirectByteBuffer* m_byteBuffer;
    float* m_data;
    int m_lockCount;
};

}
