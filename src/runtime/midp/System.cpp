#include "runtime/midp/System.h"

#include <stddef.h>

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "sdk/CoreFoundation.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)

#include <sys/time.h>

extern "C" {

int puts(const char* s);

typedef struct __CFLocale* CFLocaleRef;

CFArrayRef CFLocaleCopyPreferredLanguages(void);
CFLocaleRef CFLocaleCopyCurrent(void);
CFTypeRef CFLocaleGetValue(CFLocaleRef locale, CFStringRef key);
extern CFStringRef kCFLocaleLanguageCode;

void CFShow(CFTypeRef obj);
CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char* cStr, CFStringEncoding encoding);
CFStringRef CFStringCreateWithFormatAndArguments(CFAllocatorRef alloc, void* formatOptions,
                                                 CFStringRef format, va_list arguments);

}

class NSBundle {
public:
    static NSBundle* mainBundle();
    CFStringRef objectForInfoDictionaryKey(CFStringRef key);
};

namespace midp {

String* System::getProperty(String* key) {
    INCREF(key);
    String* value = NULL;
    if (key->equals("microedition.locale")) {
        CFArrayRef languages = CFLocaleCopyPreferredLanguages();
        if (languages != NULL) {
            if (CFArrayGetCount(languages) > 0) {
                CFStringRef language = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
                CFRetain(language);
                value = new String(language);
            }
            CFRelease(languages);
        }
        if (value == NULL) {
            CFLocaleRef locale = CFLocaleCopyCurrent();
            if (locale != NULL) {
                CFStringRef language =
                    (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode);
                CFRetain(language);
                value = new String(language);
                CFRelease(locale);
            }
        }
        if (value == NULL)
            value = new String();
    } else {
        NSBundle* bundle = NSBundle::mainBundle();
        CFStringRef found = bundle->objectForInfoDictionaryKey(key->getCFString());
        if (found != NULL) {
            CFRetain(found);
            value = new String(found);
        }
    }
    DECREF(key);
    return value;
}

long long System::currentTimeMillis() {
    timeval tv;
    gettimeofday(&tv, NULL);
    double millis = (double)tv.tv_sec * 1000.0;
    double fraction = (double)tv.tv_usec * 0.001;
    long long whole = (long long)millis;
    return (long long)((double)whole + fraction);
}

void System::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    CFStringRef formatString = CFStringCreateWithCString(NULL, format, kCFStringEncodingUTF8);
    CFStringRef text = CFStringCreateWithFormatAndArguments(NULL, NULL, formatString, args);
    CFShow(text);
    CFRelease(formatString);
    CFRelease(text);
    va_end(args);
}

void System::println(const char* text) {
    puts(text);
}

void System::arraycopy(const void* src, int srcPos, const void* dst, int dstPos, int length,
                       int elementSize) {
    int count = elementSize * length;
    if (count == 0)
        return;
    const unsigned char* from =
        static_cast<const unsigned char*>(src) + elementSize * (length + srcPos);
    unsigned char* to = static_cast<unsigned char*>(const_cast<void*>(dst)) +
                        elementSize * (length + dstPos);
    do {
        from -= 1;
        to -= 1;
        count -= 1;
        *to = *from;
    } while (count != 0);
}

void System::arraycopy(const float* src, int srcPos, float* dst, int dstPos, int length) {
    if (length < 1)
        return;
    const float* from = src + srcPos;
    float* to = dst + dstPos;
    do {
        *to = *from;
        from += 1;
        to += 1;
        length -= 1;
    } while (length != 0);
}

}
