#include "runtime/midp/InputStream.h"

#include "runtime/midp/InputStreamClass.h"
#include "runtime/midp/array.h"

namespace midp {

Class* InputStream::getClass() const {
    return new InputStreamClass();
}

int InputStream::available() const {
    return 0;
}

void InputStream::close() {}

void InputStream::mark(int) {}

bool InputStream::markSupported() const {
    return false;
}

int InputStream::read(signed char* buf, int n) {
    int count = 0;
    while (count < n) {
        int b = read();
        if (b == -1)
            return count == 0 ? -1 : count;
        buf[count] = (signed char)b;
        count += 1;
    }
    return count;
}

void InputStream::reset() {}

int InputStream::skip(long n) {
    int count = 0;
    while (count < n) {
        if (read() == -1)
            break;
        count += 1;
    }
    return count;
}

int InputStream::read(array<signed char>& buffer, int offset, int length) {
    return read(&buffer[offset], length);
}

int InputStream::read(array<signed char>& buffer) {
    return read(buffer, 0, buffer.m_length);
}

InputStream::~InputStream() {}

InputStream::InputStream() {}

int InputStream::getPosition() {
    return -1;
}

bool InputStream::seek(long, int) {
    return false;
}

int InputStream::size() {
    return 0;
}

}
