#include "runtime/nio/DirectByteBuffer.h"
#include "runtime/nio/NativeDirectFloatBuffer.h"

void operator delete[](void*) noexcept;

namespace nio {

DirectByteBuffer::DirectByteBuffer(int capacity) : ByteBuffer(capacity) {
    m_data = static_cast<signed char*>(::operator new[](static_cast<unsigned>(capacity)));
}

DirectByteBuffer::~DirectByteBuffer() {
    if (m_data)
        ::operator delete[](m_data);
}

midp::Class* DirectByteBuffer::getClass() const {
    return nullptr;
}

bool DirectByteBuffer::isReadOnly() const {
    return false;
}

FloatBuffer* DirectByteBuffer::asFloatBuffer() {
    const ByteOrder* current = order();
    if (current != ByteOrder::nativeOrder())
        return nullptr;
    return new NativeDirectFloatBuffer(this);
}

ByteBuffer* DirectByteBuffer::put(const signed char*, int) {
    return this;
}

ByteBuffer* DirectByteBuffer::putInt(int) {
    return this;
}

ByteBuffer* DirectByteBuffer::get(signed char*, int) {
    return this;
}

int DirectByteBuffer::getInt() {
    const ByteOrder* byteOrder = order();
    signed char* data = m_data;
    int value = byteOrder->getInt(data + position());
    position(position() + 4);
    return value;
}

}
