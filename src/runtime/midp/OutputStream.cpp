#include "runtime/midp/OutputStream.h"

#include "runtime/midp/File.h"
#include "runtime/midp/FileOutputStreamIPhone.h"
#include "runtime/midp/OutputStreamClass.h"
#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"
#include "runtime/midp/array.h"

namespace midp {

OutputStream* OutputStream::getResourceAsStream(String* name) {
    ReferenceCountedHardReference nameRef(name);
    ReferenceCountedPointer<String> path(File::getDocumentsFilename(name));
    FileOutputStreamIPhone* stream = new FileOutputStreamIPhone(path);
    if (stream->loadSuccessful())
        return stream;
    delete stream;
    return NULL;
}

Class* OutputStream::getClass() const {
    return new OutputStreamClass();
}

void OutputStream::write(array<signed char>& buffer) {
    write(buffer, 0, buffer.m_length);
}

void OutputStream::write(const void* data, int length) {
    const unsigned char* bytes = (const unsigned char*)data;
    while (length != 0) {
        write(*bytes);
        bytes += 1;
        length -= 1;
    }
}

bool OutputStream::close() {
    return true;
}

void OutputStream::write(array<signed char>& buffer, int offset, int length) {
    while (length != 0) {
        write((unsigned char)buffer[offset]);
        offset += 1;
        length -= 1;
    }
}

OutputStream::~OutputStream() {}

OutputStream::OutputStream() {}

}
