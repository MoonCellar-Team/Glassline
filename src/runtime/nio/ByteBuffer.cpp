#include "runtime/nio/ByteBuffer.h"
#include "runtime/nio/DirectByteBuffer.h"

namespace nio {

ByteBuffer::ByteBuffer(int capacity) : Buffer(capacity), m_order(ByteOrder::BIG_ENDIAN) {
    limit(capacity);
}

ByteBuffer::~ByteBuffer() {}

ByteBuffer* ByteBuffer::allocateDirect(int bytes) {
    return new DirectByteBuffer(bytes);
}

const ByteOrder* ByteBuffer::order() const {
    return m_order;
}

ByteBuffer* ByteBuffer::order(const ByteOrder* value) {
    m_order = value;
    return this;
}

}
