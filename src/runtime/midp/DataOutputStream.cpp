#include "runtime/midp/DataOutputStream.h"

#include "runtime/midp/DataOutputStreamClass.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "runtime/midp/array.h"

namespace midp {

Class* DataOutputStream::getClass() const {
    return new DataOutputStreamClass();
}

bool DataOutputStream::close() {
    if (m_out == NULL)
        return false;
    bool ok = m_out->close();
    DECREF(m_out);
    m_out = NULL;
    return ok;
}

void DataOutputStream::write(unsigned char v) {
    m_out->write(v);
}

void DataOutputStream::write(array<signed char>& buffer) {
    m_out->write(buffer);
}

void DataOutputStream::write(array<signed char>& buffer, int offset, int length) {
    m_out->write(buffer, offset, length);
}

void DataOutputStream::write(const void* bytes, int length) {
    m_out->write(bytes, length);
}

void DataOutputStream::writeBoolean(bool v) {
    write((unsigned char)v);
}

void DataOutputStream::writeByte(unsigned char v) {
    write(v);
}

void DataOutputStream::writeFloat(float v) {
    unsigned char bytes[4];
    unsigned char* raw = (unsigned char*)&v;
    bytes[0] = raw[3];
    bytes[1] = raw[2];
    bytes[2] = raw[1];
    bytes[3] = raw[0];
    write(bytes[0]);
    write(bytes[1]);
    write(bytes[2]);
    write(bytes[3]);
}

void DataOutputStream::writeInt(int v) {
    unsigned char bytes[4];
    bytes[0] = (unsigned char)((unsigned)v >> 24);
    bytes[1] = (unsigned char)((unsigned)v >> 16);
    bytes[2] = (unsigned char)((unsigned)v >> 8);
    bytes[3] = (unsigned char)v;
    write(bytes, 4);
}

void DataOutputStream::writeShort(short v) {
    unsigned char bytes[2];
    bytes[0] = (unsigned char)((unsigned short)v >> 8);
    bytes[1] = (unsigned char)v;
    write(bytes, 2);
}

void DataOutputStream::writeUnsignedShort(unsigned short v) {
    unsigned char bytes[2];
    bytes[0] = (unsigned char)(v >> 8);
    bytes[1] = (unsigned char)v;
    write(bytes, 2);
}

void DataOutputStream::writeUTF(String* str) {
    INCREF(str);
    if (str == NULL) {
        writeUnsignedShort(0);
    } else {
        array<signed char> bytes = str->getBytes("UTF-8");
        writeUnsignedShort((unsigned short)bytes.m_length);
        if (bytes.m_length >= 1)
            write(bytes.data(), bytes.m_length);
    }
    DECREF(str);
}

DataOutputStream::~DataOutputStream() {
    close();
}

DataOutputStream::DataOutputStream(OutputStream* out) : m_out(out) {
    INCREF(out);
}

}
