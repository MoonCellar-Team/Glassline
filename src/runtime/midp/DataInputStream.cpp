#include "runtime/midp/DataInputStream.h"

#include "runtime/midp/DataInputStreamClass.h"
#include "runtime/midp/String.h"
#include "runtime/midp/array.h"

namespace midp {

bool DataInputStream::eofExceptionThrown() {
    return m_eof;
}

Class* DataInputStream::getClass() const {
    return new DataInputStreamClass();
}

void DataInputStream::close() {
    m_in->close();
}

signed char DataInputStream::readByte() {
    int value = read();
    if (value == -1)
        m_eof = true;
    return (signed char)value;
}

bool DataInputStream::readBoolean() {
    return (readByte() & 0xff) != 0;
}

int DataInputStream::readUnsignedByte() {
    return (unsigned char)readByte();
}

void DataInputStream::readFully(signed char* bytes, int length) {
    signed char* end = bytes + length;
    while (bytes < end) {
        int n = m_in->read(bytes, (int)(end - bytes));
        if (n <= 0) {
            m_eof = true;
            return;
        }
        bytes += n;
    }
}

short DataInputStream::readShort() {
    unsigned char bytes[2];
    readFully((signed char*)bytes, 2);
    return (short)((bytes[0] << 8) | bytes[1]);
}

int DataInputStream::readUnsignedShort() {
    return (unsigned short)readShort();
}

int DataInputStream::readInt() {
    unsigned char bytes[4];
    readFully((signed char*)bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

float DataInputStream::readFloat() {
    union {
        int bits;
        float value;
    } pun;
    pun.bits = readInt();
    return pun.value;
}

int DataInputStream::available() const {
    return m_in->available();
}

void DataInputStream::mark(int readLimit) {
    m_in->mark(readLimit);
}

bool DataInputStream::markSupported() const {
    return m_in->markSupported();
}

int DataInputStream::read() {
    return m_in->read();
}

int DataInputStream::read(signed char* bytes, int length) {
    return m_in->read(bytes, length);
}

void DataInputStream::reset() {
    m_in->reset();
}

int DataInputStream::skip(long length) {
    return m_in->skip(length);
}

int DataInputStream::getPosition() {
    return m_in->getPosition();
}

int DataInputStream::size() {
    return m_in->size();
}

bool DataInputStream::seek(long offset, int whence) {
    return m_in->seek(offset, whence);
}

void DataInputStream::readFully(array<signed char>& bytes, int offset, int length) {
    if (length < 1)
        return;
    readFully(&bytes[offset], length);
}

void DataInputStream::readFully(array<signed char>& bytes) {
    readFully(bytes, 0, bytes.m_length);
}

void DataInputStream::readFully(array<unsigned char>& bytes, int offset, int length) {
    if (length < 1)
        return;
    readFully(reinterpret_cast<signed char*>(&bytes[offset]), length);
}

void DataInputStream::readFully(array<unsigned char>& bytes) {
    readFully(bytes, 0, bytes.m_length);
}

String* DataInputStream::readUTF() {
    if (m_in->available() <= 1)
        return new String();
    int length = readUnsignedShort();
    if (length == 0)
        return new String();
    int avail = m_in->available();
    if (avail < length) {
        m_in->skip(avail);
        return new String();
    }
    array<signed char> bytes(length);
    readFully(bytes);
    return new String(bytes, "UTF-8");
}

DataInputStream::~DataInputStream() {
    DECREF(m_in);
}

DataInputStream::DataInputStream(InputStream* in) : m_in(in), m_eof(false) {
    INCREF(m_in);
}

}
