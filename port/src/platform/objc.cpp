#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "port/objc.h"

namespace port {
namespace objc {

namespace {

std::mutex& liveMutex() {
    static std::mutex m;
    return m;
}
std::unordered_set<const void*>& live() {
    static std::unordered_set<const void*> s;
    return s;
}
std::vector<Responder>& responders() {
    static std::vector<Responder> v;
    return v;
}

const std::thread::id s_mainThread = std::this_thread::get_id();

struct Pending {
    std::function<void()> fn;
    bool done = false;
};
std::mutex s_queueMutex;
std::condition_variable s_queueDone;
std::deque<std::shared_ptr<Pending>> s_queue;
void (*s_wake)() = nullptr;

[[noreturn]] void unknown(const Message& m) {
    const char* cls = isObject(m.self) ? static_cast<const Object*>(m.self)->cls()->name : "?";
    fprintf(stderr, "medecomp: не реализовано: -[%s %s]\n", cls, m.sel);
    abort();
}

void* retainImpl(Object* self, Message&) {
    CFRetain(self);
    return self;
}
void* releaseImpl(Object* self, Message&) {
    CFRelease(self);
    return nullptr;
}
void* selfImpl(Object* self, Message&) { return self; }
void* performImpl(Object* self, Message& m);
}

const Class NSObjectClass = {
    {"NSObject", destroyAs<Object>, nullptr},
    nullptr,
    {
        {"retain", retainImpl},
        {"release", releaseImpl},
        {"autorelease", selfImpl},
        {"init", selfImpl},
        {"performSelectorOnMainThread:withObject:waitUntilDone:", performImpl},
    },
};

namespace {
void* performImpl(Object* self, Message& m) {
    const char* sel = va_arg(*m.args, const char*);
    void* arg = va_arg(*m.args, void*);
    bool wait = va_arg(*m.args, int) != 0;
    perform(self, sel, arg, wait);
    return nullptr;
}
}

Object::Object(const Class* c) : cf::Object(c) {
    std::lock_guard<std::mutex> l(liveMutex());
    live().insert(this);
}

Object::~Object() {
    std::lock_guard<std::mutex> l(liveMutex());
    live().erase(this);
}

bool isObject(const void* p) {
    if (!p) return false;
    std::lock_guard<std::mutex> l(liveMutex());
    return live().count(p) != 0;
}

void addResponder(Responder r) { responders().push_back(r); }

void* sendv(void* self, const char* sel, va_list args, void* stret) {
    if (!self) return nullptr;
    if (isObject(self)) {
        Object* o = static_cast<Object*>(self);
        for (const Class* c = o->cls(); c; c = c->super) {
            auto it = c->methods.find(sel);
            if (it == c->methods.end()) continue;
            va_list ap;
            va_copy(ap, args);
            Message m{self, sel, &ap, stret};
            void* r = it->second(o, m);
            va_end(ap);
            return r;
        }
    }
    for (Responder r : responders()) {
        va_list ap;
        va_copy(ap, args);
        Message m{self, sel, &ap, stret};
        void* result = nullptr;
        bool ok = r(m, &result);
        va_end(ap);
        if (ok) return result;
    }
    Message m{self, sel, nullptr, stret};
    unknown(m);
}

void* send(void* self, const char* sel, ...) {
    va_list ap;
    va_start(ap, sel);
    void* r = sendv(self, sel, ap);
    va_end(ap);
    return r;
}

void perform(void* self, const char* sel, void* arg, bool wait) {
    bool held = isObject(self);
    if (held) CFRetain(self);
    onMainThread([self, sel, arg, held] {
        send(self, sel, arg);
        if (held) CFRelease(self);
    }, wait);
}

bool isMainThread() { return std::this_thread::get_id() == s_mainThread; }

void setMainThreadWaker(void (*wake)()) { s_wake = wake; }

void onMainThread(std::function<void()> fn, bool wait) {
    if (wait && isMainThread()) {
        fn();
        return;
    }
    auto p = std::make_shared<Pending>();
    p->fn = std::move(fn);
    {
        std::lock_guard<std::mutex> l(s_queueMutex);
        s_queue.push_back(p);
    }
    if (s_wake) s_wake();
    if (wait) {
        std::unique_lock<std::mutex> l(s_queueMutex);
        s_queueDone.wait(l, [&] { return p->done; });
    }
}

bool runMainThreadQueue() {
    std::deque<std::shared_ptr<Pending>> batch;
    {
        std::lock_guard<std::mutex> l(s_queueMutex);
        batch.swap(s_queue);
    }
    for (auto& p : batch) {
        p->fn();
        std::lock_guard<std::mutex> l(s_queueMutex);
        p->done = true;
    }
    if (!batch.empty()) s_queueDone.notify_all();
    return !batch.empty();
}

}
}

using port::objc::sendv;

extern "C" {

void* objc_msgSend(void* self, const char* sel, ...) {
    va_list ap;
    va_start(ap, sel);
    void* r = sendv(self, sel, ap);
    va_end(ap);
    return r;
}

void objc_msgSend_stret(void* result, void* self, const char* sel, ...) {
    if (!self) return;
    va_list ap;
    va_start(ap, sel);
    sendv(self, sel, ap, result);
    va_end(ap);
}

void NSObject_performSelectorOnMainThread(void* self, const char* sel, void* arg, bool wait)
    MEDECOMP_SYMBOL("_ZN8NSObject52performSelectorOnMainThread_withObject_waitUntilDoneEP13objc_selectorPvb");
void NSObject_performSelectorOnMainThread(void* self, const char* sel, void* arg, bool wait) {
    port::objc::perform(self, sel, arg, wait);
}

}
