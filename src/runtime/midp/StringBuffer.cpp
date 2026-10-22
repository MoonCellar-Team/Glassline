#include "runtime/midp/StringBuffer.h"

#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

namespace midp {

StringBuffer::StringBuffer() : m_string(NULL) {
    m_string = CFStringCreateMutable(kCFAllocatorDefault, 0);
}

Class* StringBuffer::getClass() const {
    return NULL;
}

int StringBuffer::length() const {
    return (int)CFStringGetLength(m_string);
}

String* StringBuffer::toString() const {
    if (length() >= 1)
        return new String(m_string);
    return new String("");
}

void StringBuffer::setLength(int newLength) {
    int current = length();
    if (current <= newLength)
        return;
    CFRange range;
    range.location = newLength;
    range.length = current - newLength;
    CFStringDelete(m_string, range);
}

void StringBuffer::copyCString(char* output, unsigned capacity) const {
    CFStringGetCString(m_string, output, (CFIndex)capacity, kCFStringEncodingUTF8);
}

StringBuffer* StringBuffer::append(String* value) {
    ReferenceCountedHardReference hold(value);
    CFStringAppend(m_string, value->getCFString());
    return this;
}

StringBuffer* StringBuffer::append(unsigned value) {
    CFStringAppendFormat(m_string, NULL, CFSTR("%u"), value);
    return this;
}

StringBuffer* StringBuffer::append(int value) {
    CFStringAppendFormat(m_string, NULL, CFSTR("%i"), value);
    return this;
}

StringBuffer* StringBuffer::append(unsigned short c) {
    CFStringAppendFormat(m_string, NULL, CFSTR("%C"), c);
    return this;
}

StringBuffer* StringBuffer::append(char c) {
    CFStringAppendFormat(m_string, NULL, CFSTR("%c"), c);
    return this;
}

StringBuffer* StringBuffer::append(const char* value) {
    CFStringAppendCString(m_string, value, kCFStringEncodingASCII);
    return this;
}

StringBuffer::~StringBuffer() {
    if (m_string != NULL)
        CFRelease(m_string);
    m_string = NULL;
}

StringBuffer::StringBuffer(int) : m_string(NULL) {
    m_string = CFStringCreateMutable(kCFAllocatorDefault, 0);
}

}
