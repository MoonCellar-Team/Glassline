#include "runtime/midp/String.h"

#include "runtime/midp/ReferenceCountedHardReference.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/StringClass.h"
#include "runtime/midp/array.h"
#include <stddef.h>

extern "C" int strcmp(const char* a, const char* b);

void* operator new(size_t);

extern "C" {

typedef long CFIndex;
typedef unsigned int CFStringEncoding;
typedef const void* CFTypeRef;
typedef const struct __CFString* CFStringRef;

struct CFRange {
    CFIndex location;
    CFIndex length;
};

enum {
    kCFStringEncodingASCII = 0x0600,
    kCFStringEncodingUTF8 = 0x08000100
};

extern const void* kCFAllocatorNull;

CFTypeRef CFRetain(CFTypeRef cf);
void CFRelease(CFTypeRef cf);
CFIndex CFStringGetLength(CFStringRef string);
CFIndex CFStringGetCharacterAtIndex(CFStringRef string, CFIndex index);
int CFStringCompare(CFStringRef a, CFStringRef b, unsigned int options);
int CFStringCompareWithOptions(CFStringRef a, CFStringRef b, CFRange range, unsigned int options);
CFRange CFStringFind(CFStringRef string, CFStringRef toFind, unsigned int options);
CFStringRef CFStringCreateWithCString(const void* alloc, const char* cStr, CFStringEncoding encoding);
CFStringRef CFStringCreateWithCStringNoCopy(const void* alloc, const char* cStr, CFStringEncoding encoding,
                                            const void* contentsDeallocator);
CFStringRef CFStringCreateWithBytes(const void* alloc, const unsigned char* bytes, CFIndex numBytes,
                                    CFStringEncoding encoding, bool isExternalRepresentation);
CFStringRef CFStringCreateWithSubstring(const void* alloc, CFStringRef string, CFRange range);
CFStringRef CFStringCreateCopy(const void* alloc, CFStringRef string);
CFStringRef CFStringCreateWithFormat(const void* alloc, const void* formatOptions, CFStringRef format, ...);
CFStringRef CFStringCreateMutable(const void* alloc, CFIndex maxLength);
void CFStringAppend(CFStringRef theString, CFStringRef appended);
void CFStringUppercase(CFStringRef theString, const void* locale);
CFIndex CFStringGetBytes(CFStringRef theString, CFRange range, CFStringEncoding encoding, unsigned char lossByte,
                         bool isExternalRepresentation, unsigned char* buffer, CFIndex maxBufLen, CFIndex* usedBufLen);

#define CFSTR(s) ((CFStringRef)__builtin___CFStringMakeConstantString(s))

}

namespace midp {

const __CFString* String::getCFString() const {
    return m_string;
}

Class* String::getClass() const {
    return new StringClass();
}

String::String(const __CFString* s) : m_string(NULL), m_hashValid(false), m_hash(0) {
    if (s != NULL)
        set(s);
}

String::~String() {
    if (m_string != NULL)
        CFRelease(m_string);
    set(NULL);
}

bool String::equals(const Object* other) const {
    const String* s = static_cast<const String*>(other);
    return CFStringCompare(getCFString(), s->getCFString(), 0) == 0;
}

int String::hashCode() const {
    if (!m_hashValid) {
        int hash = 0;
        int len = length();
        if (len >= 1) {
            int mult = 1;
            for (int i = len - 1;; i--) {
                hash += static_cast<int>(charAt(i)) * mult;
                mult *= 31;
                if (i == 0)
                    break;
            }
        }
        m_hash = hash;
        m_hashValid = true;
    }
    return m_hash;
}

String::String() : m_string(NULL), m_hashValid(false), m_hash(0) {}

int String::length() const {
    const __CFString* s = getCFString();
    if (s == NULL)
        return 0;
    return static_cast<int>(CFStringGetLength(s));
}

array<signed char> String::getBytes(const char* encoding) {
    if (m_string == NULL || length() < 1)
        return array<signed char>();

    CFStringEncoding enc = kCFStringEncodingASCII;
    if (encoding != NULL && strcmp(encoding, "UTF-8") == 0)
        enc = kCFStringEncodingUTF8;

    unsigned char buf[512];
    CFIndex used = 0;
    CFRange range;
    range.location = 0;
    range.length = length();
    CFStringGetBytes(m_string, range, enc, 0, false, buf, 512, &used);

    array<signed char> bytes(static_cast<int>(used));
    for (int i = 0; i < used; i++)
        bytes[i] = static_cast<signed char>(buf[i]);
    return bytes;
}

bool String::equals(const __CFString* other) const {
    const __CFString* mine = getCFString();
    if (mine == other)
        return true;
    int myLen = length();
    int otherLen = 0;
    if (other != NULL)
        otherLen = static_cast<int>(CFStringGetLength(other));
    if (myLen != otherLen)
        return false;
    if ((otherLen | myLen) == 0)
        return true;
    return CFStringCompare(mine, other, 0) == 0;
}

bool String::equals(String* other) const {
    ReferenceCountedHardReference hold(other);
    if (other == NULL || other == this)
        return other != NULL;
    return equals(other->getCFString());
}

bool String::equals(const char* s) const {
    if (s == NULL)
        return false;
    const __CFString* cf = CFStringCreateWithCStringNoCopy(NULL, s, kCFStringEncodingASCII, kCFAllocatorNull);
    bool same = equals(cf);
    CFRelease(cf);
    return same;
}

bool String::startsWith(String* prefix, int offset) const {
    ReferenceCountedPointer<String> hold(prefix);
    const __CFString* mine = getCFString();
    int len = length();
    const __CFString* theirs = prefix->getCFString();
    int prefixLen = prefix->length();
    if (prefixLen > len || len <= offset)
        return false;
    CFRange range;
    range.location = offset;
    range.length = prefixLen;
    return CFStringCompareWithOptions(mine, theirs, range, 0) == 0;
}

bool String::startsWith(String* prefix) const {
    return startsWith(prefix, 0);
}

String* String::substring(int begin, int end) const {
    String* s = new String();
    if (length() > begin) {
        CFRange range;
        range.location = begin;
        range.length = end - begin;
        s->set(CFStringCreateWithSubstring(NULL, getCFString(), range));
    }
    return s;
}

unsigned short String::charAt(int index) const {
    return static_cast<unsigned short>(CFStringGetCharacterAtIndex(getCFString(), index));
}

int String::lastIndexOf(unsigned short ch, int from) const {
    int len = length();
    int i = from;
    if (from >= len)
        i = len - 1;
    for (; i >= 0; i--) {
        if (charAt(i) == ch)
            return i;
    }
    return -1;
}

int String::lastIndexOf(unsigned short ch) const {
    return lastIndexOf(ch, length() - 1);
}

int String::indexOf(String* str, int) const {
    ReferenceCountedHardReference hold(str);
    CFRange found = CFStringFind(getCFString(), str->getCFString(), 0);
    return static_cast<int>(found.location);
}

String* String::valueOf(int value) {
    String* s = new String();
    s->set(CFStringCreateWithFormat(NULL, NULL, CFSTR("%i"), value));
    return s;
}

String::String(const array<signed char>& bytes, const char* encoding)
    : m_string(NULL), m_hashValid(false), m_hash(0) {
    if (bytes.m_length < 1)
        return;
    CFStringEncoding enc = kCFStringEncodingASCII;
    if (encoding != NULL && strcmp(encoding, "UTF-8") == 0)
        enc = kCFStringEncodingUTF8;
    const unsigned char* data = reinterpret_cast<const unsigned char*>(&bytes[0]);
    set(CFStringCreateWithBytes(NULL, data, bytes.m_length, enc, false));
}

String::String(const char* s) : m_string(NULL), m_hashValid(false), m_hash(0) {
    if (s == NULL)
        return;
    set(CFStringCreateWithCString(NULL, s, kCFStringEncodingASCII));
}

String::String(String* other) : m_string(NULL), m_hashValid(false), m_hash(0) {
    const __CFString* s = other->getCFString();
    if (s == NULL)
        return;
    CFRetain(s);
    set(s);
}

String::String(__CFString* s) : m_string(NULL), m_hashValid(false), m_hash(0) {
    if (s == NULL)
        return;
    set(CFStringCreateCopy(NULL, s));
}

String* String::toUpperCase() const {
    CFStringRef upper = CFStringCreateMutable(NULL, 0);
    CFStringAppend(upper, m_string);
    CFStringUppercase(upper, NULL);
    String* s = new String(const_cast<__CFString*>(upper));
    CFRelease(upper);
    return s;
}

void String::set(const __CFString* s) {
    m_string = s;
}

}
