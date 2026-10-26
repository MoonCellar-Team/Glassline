#include <math.h>
#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#endif

#include <atomic>
#include <cmath>

#include "game/PhotoPicker.h"
#include "platform/PhotoPickerImpl.h"
#include "port/objc.h"
#include "port/platform.h"
#include "runtime/midp/AccelerometerIPhone.h"

using port::objc::Class;
using port::objc::Message;
using port::objc::NSObjectClass;
using port::objc::Object;

namespace {

struct AccelerometerObject : Object {
    midp::AccelerometerIPhone* owner = nullptr;
    AccelerometerObject();
    ~AccelerometerObject();
};

const Class kAccelerometer = {
    {"iPhoneAccelerometer", port::objc::destroyAs<AccelerometerObject>, nullptr}, &NSObjectClass, {}};

AccelerometerObject* s_delegate;
std::atomic<float> s_tilt{0.0f};

AccelerometerObject::AccelerometerObject() : Object(&kAccelerometer) {}
AccelerometerObject::~AccelerometerObject() {
    if (s_delegate == this) s_delegate = nullptr;
}

struct PickerObject : Object {
    PhotoPicker* manager = nullptr;
    PickerObject();
};

void* pickerShow(Object* self, Message&) {
    CFRetain(self);
    port::objc::onMainThread([self] {
        static_cast<PickerObject*>(self)->manager->photoCancel();
        CFRelease(self);
    }, false);
    return nullptr;
}

void* pickerHide(Object*, Message&) { return nullptr; }

void* pickerSaved(Object* self, Message& m) {
    va_arg(*m.args, void*);
    void* error = va_arg(*m.args, void*);
    PhotoPicker* manager = static_cast<PickerObject*>(self)->manager;
    manager->m_saveFailed = error != nullptr;
    manager->m_saveDone = true;
    return nullptr;
}

const Class kPhotoPicker = {
    {"PhotoPickerImpl", port::objc::destroyAs<PickerObject>, nullptr},
    &NSObjectClass,
    {{"show", pickerShow}, {"hide", pickerHide}, {"image:didFinishSavingWithError:contextInfo:", pickerSaved}},
};

PickerObject::PickerObject() : Object(&kPhotoPicker) {}

}

iPhoneAccelerometer* iPhoneAccelerometer::alloc() {
    return reinterpret_cast<iPhoneAccelerometer*>(new AccelerometerObject());
}

iPhoneAccelerometer* iPhoneAccelerometer::initWithCppInterface(midp::AccelerometerIPhone* owner) {
    AccelerometerObject* a = reinterpret_cast<AccelerometerObject*>(this);
    a->owner = owner;
    s_delegate = a;
    return this;
}

void iPhoneAccelerometer::release() { CFRelease(this); }

PhotoPickerImpl* PhotoPickerImpl::alloc() { return reinterpret_cast<PhotoPickerImpl*>(new PickerObject()); }

PhotoPickerImpl* PhotoPickerImpl::initWithManager(PhotoPicker* manager) {
    reinterpret_cast<PickerObject*>(this)->manager = manager;
    return this;
}

namespace port {
namespace platform {

void setTilt(float tilt) { s_tilt = tilt < -1 ? -1 : tilt > 1 ? 1 : tilt; }

float tilt() { return s_tilt; }

void accelerate(float tilt) {
    if (!s_delegate || !s_delegate->owner) return;
    float a = tilt * float(M_PI / 2);
    s_delegate->owner->accelerate(0.0f, -sinf(a), -cosf(a));
}

void runAccelerometer() {
#ifdef __EMSCRIPTEN__
    double t = MAIN_THREAD_EM_ASM_DOUBLE({ return Module.medecompTilt; });
    if (!std::isnan(t)) {
        static bool told;
        if (!told) fprintf(stderr, "medecomp: наклон — от устройства (deviceorientation), сейчас %.2f\n", t);
        told = true;
        setTilt(float(t));
    }
#endif
    accelerate(s_tilt);
}

}
}
