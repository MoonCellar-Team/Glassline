#pragma once

#include <stdarg.h>

#include <functional>
#include <map>
#include <string>

#include "port/cf.h"

namespace port {
namespace objc {

struct Message {
    void* self;
    const char* sel;
    va_list* args;
    void* stret;
};

struct Object;
using Method = void* (*)(Object* self, Message& m);

struct Class : cf::Class {
    const Class* super;
    std::map<std::string, Method> methods;
    Class(cf::Class base, const Class* super, std::map<std::string, Method> methods)
        : cf::Class(base), super(super), methods(std::move(methods)) {}
};
extern const Class NSObjectClass;

struct Object : cf::Object {
    explicit Object(const Class* c);
    ~Object();
    const Class* cls() const { return static_cast<const Class*>(isa); }
};
template <class T>
void destroyAs(cf::Object* o) { delete static_cast<T*>(o); }

bool isObject(const void* p);

using Responder = bool (*)(Message& m, void** result);
void addResponder(Responder r);

void* send(void* self, const char* sel, ...);
void* sendv(void* self, const char* sel, va_list args, void* stret = nullptr);
void perform(void* self, const char* sel, void* arg, bool wait);

bool isMainThread();
void onMainThread(std::function<void()> fn, bool wait);
bool runMainThreadQueue();
void setMainThreadWaker(void (*wake)());

}
}
