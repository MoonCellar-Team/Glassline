#include "runtime/midp/ByteArrayOutputStream.h"

#include "runtime/midp/ByteArrayOutputStreamClass.h"
#include "runtime/midp/System.h"

namespace midp {

Class* ByteArrayOutputStream::getClass() const {
    return new ByteArrayOutputStreamClass();
}

bool ByteArrayOutputStream::close() {
    return true;
}

void ByteArrayOutputStream::writeTo(OutputStream* out) {
    out->write(m_buf, 0, m_count);
}

void ByteArrayOutputStream::ensureCapacity(int minCapacity) {
    int oldLength = m_buf.m_length;
    if (oldLength >= minCapacity)
        return;
    array<signed char> old(m_buf);
    array<signed char> grown(minCapacity);
    m_buf = grown;
    System::arraycopy(old.data(), 0, m_buf.data(), 0, oldLength, 1);
}

void ByteArrayOutputStream::write(const void* data, int length) {
    ensureCapacity(m_count + length);
    System::arraycopy(data, 0, m_buf.data(), m_count, length, 1);
    m_count += length;
}

void ByteArrayOutputStream::write(unsigned char b) {
    if (m_count == m_buf.m_length)
        ensureCapacity(m_buf.m_length << 1);
    int index = m_count;
    m_count = index + 1;
    m_buf[index] = (signed char)b;
}

ByteArrayOutputStream::~ByteArrayOutputStream() {}

ByteArrayOutputStream::ByteArrayOutputStream() : m_buf(32), m_count(0) {}

void ByteArrayOutputStream::reset() {
    m_count = 0;
}

int ByteArrayOutputStream::size() {
    return m_count;
}

}
