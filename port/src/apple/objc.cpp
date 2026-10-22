#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <mutex>
#include <set>
#include <string>

#include "port/cf.h"

extern "C" {

const void* CFStringCreateWithFormatAndArguments(const void* alloc, const void* options, const void* format, va_list ap);

const void* sel_registerName(const char* name) {
    static std::mutex m;
    static std::set<std::string> names;
    std::lock_guard<std::mutex> lock(m);
    return names.insert(name ? name : "").first->c_str();
}

void* NSClassFromString(const void*) { return nullptr; }

void NSLog(const void* format, ...) {
    va_list ap;
    va_start(ap, format);
    const void* s = CFStringCreateWithFormatAndArguments(nullptr, nullptr, format, ap);
    va_end(ap);
    fprintf(stderr, "%s\n", port::cf::utf8(s).c_str());
    CFRelease(s);
}

void objc_enumerationMutation(void*) {
    fprintf(stderr, "medecomp: коллекция изменена во время перебора\n");
    abort();
}

int __srget(FILE* f) { return getc(f); }

int sysctl(int*, unsigned, void*, size_t*, void*, size_t) {
    errno = ENOSYS;
    return -1;
}

}
