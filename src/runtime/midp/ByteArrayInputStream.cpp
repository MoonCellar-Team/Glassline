#include "runtime/midp/ByteArrayInputStream.h"

#include "runtime/midp/ByteArrayInputStreamClass.h"
#include "sdk/string.h"

namespace midp {

Class* ByteArrayInputStream::getClass() const {
    return new ByteArrayInputStreamClass();
}

int ByteArrayInputStream::available() const {
    return m_count - m_pos;
}

void ByteArrayInputStream::mark(int) {
    m_mark = m_pos;
}

bool ByteArrayInputStream::markSupported() const {
    return true;
}

void ByteArrayInputStream::reset() {
    m_pos = m_mark;
}

int ByteArrayInputStream::skip(long n) {
    int avail = m_count - m_pos;
    if (avail < n)
        n = avail;
    m_pos += (int)n;
    return (int)n;
}

int ByteArrayInputStream::getPosition() {
    return m_pos;
}

int ByteArrayInputStream::size() {
    return m_count;
}

bool ByteArrayInputStream::seek(long offset, int whence) {
    if (whence == 1)
        offset += m_pos;
    else if (whence == 2)
        offset += m_count;
    else if (whence != 0)
        return false;
    int requested = (int)offset;
    int pos = requested;
    if (pos < 0)
        pos = 0;
    if (pos > m_count)
        pos = m_count;
    m_pos = pos;
    return requested >= 0 && requested <= m_count;
}

int ByteArrayInputStream::read() {
    if (m_pos == m_count)
        return -1;
    int index = m_off + m_pos;
    m_pos += 1;
    return (unsigned char)m_buf[index];
}

int ByteArrayInputStream::read(signed char* buf, int n) {
    int avail = m_count - m_pos;
    if (avail < n)
        n = avail;
    memcpy(buf, &m_buf[m_pos + m_off], (size_t)n);
    m_pos += n;
    return n;
}

ByteArrayInputStream::~ByteArrayInputStream() {}

ByteArrayInputStream::ByteArrayInputStream(array<signed char>& bytes)
    : m_buf(bytes), m_off(0), m_count(bytes.m_length), m_mark(0), m_pos(0) {}

}
