#include "runtime/nio/Buffer.h"

namespace nio {

Buffer::Buffer(int capacity)
    : m_capacity(capacity), m_limit(0), m_position(0), m_mark(-1) {}

Buffer::~Buffer() {}

midp::Class* Buffer::getClass() const {
    return nullptr;
}

int Buffer::capacity() const {
    return m_capacity;
}

int Buffer::position() const {
    return m_position;
}

int Buffer::limit() const {
    return m_limit;
}

int Buffer::remaining() const {
    int lim = limit();
    return lim - position();
}

Buffer* Buffer::position(int value) {
    m_position = value;
    return this;
}

Buffer* Buffer::limit(int value) {
    m_limit = value;
    return this;
}

Buffer* Buffer::rewind() {
    position(0);
    clearMark();
    return this;
}

void Buffer::clearMark() {
    m_mark = -1;
}

void Buffer::requireValidIndex(int) const {}

}
