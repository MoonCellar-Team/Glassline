#include "runtime/nio/NativeDirectFloatBuffer.h"

#include "game/MonkeyApp.h"
#include "m3g/Placement.h"

void* operator new(__SIZE_TYPE__);

namespace nio {

NativeDirectFloatBuffer::NativeDirectFloatBuffer(DirectByteBuffer* buffer)
    : FloatBuffer(buffer->remaining() >> 2),
      m_byteBuffer(buffer),
      m_data(reinterpret_cast<float*>(buffer->m_data + (buffer->position() & ~3))),
      m_lockCount(0) {
    midp::INCREF(buffer);
}

NativeDirectFloatBuffer::~NativeDirectFloatBuffer() {
    midp::DECREF(m_byteBuffer);
}

bool NativeDirectFloatBuffer::isReadOnly() const {
    return false;
}

const ByteOrder* NativeDirectFloatBuffer::order() const {
    return ByteOrder::nativeOrder();
}

FloatBuffer* NativeDirectFloatBuffer::put(float value) {
    int pos = position();
    position(pos + 1);
    m_data[pos] = value;
    return this;
}

FloatBuffer* NativeDirectFloatBuffer::put(int index, float value) {
    requireValidIndex(index);
    m_data[index] = value;
    return this;
}

FloatBuffer* NativeDirectFloatBuffer::put(const float* src, int offset, int length) {
    int pos = position();
    position(pos + length);
    if (length >= 1) {
        const float* in = src + offset;
        float* out = m_data + pos;
        do {
            *out = *in;
            ++in;
            ++out;
            --length;
        } while (length != 0);
    }
    return this;
}

float* NativeDirectFloatBuffer::lockBuffer() {
    m_lockCount += 1;
    return m_data;
}

void NativeDirectFloatBuffer::unlock() {
    m_lockCount -= 1;
}

}

MonkeyApp* implementation_getMIDlet() {
    return new MonkeyApp();
}
