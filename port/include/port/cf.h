#pragma once

#include <atomic>
#include <string>

namespace port {
namespace cf {

struct Object;

struct Class {
    const char* name;
    void (*destroy)(Object* object);
    std::u16string (*describe)(const Object* o);
};

struct Object {
    const Class* isa;
    std::atomic<int> refs;
    explicit Object(const Class* c) : isa(c), refs(1) {}
};

bool isString(const void* object);
std::u16string text(const void* string);
std::string utf8(const void* string);
const void* makeString(const std::u16string& s);
const void* makeString(const std::string& utf8);
std::u16string fromUtf8(const char* s, size_t n);
std::string toUtf8(const std::u16string& s);

std::u16string describe(const void* object);

const std::string& bundlePath();
std::string infoString(const char* key);

}
}

extern "C" {
const void* CFRetain(const void* object);
void CFRelease(const void* object);
}
