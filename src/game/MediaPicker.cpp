#include "game/MediaPicker.h"

#include <stddef.h>

#include "platform/MediaPickerImpl.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/RuntimeIPhone.h"

void MediaPicker::hideNotify() {
    m_impl->performSelectorOnMainThread_withObject_waitUntilDone(sel_registerName("hide"), NULL, true);
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->resumeAudio();
}

void MediaPicker::showNotify() {
    m_impl->performSelectorOnMainThread_withObject_waitUntilDone(sel_registerName("show"), NULL, true);
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->suspendAudio();
}

void MediaPicker::deactivate() {
    if (m_active) {
        m_display->setCurrent(m_previous);
        m_previous = midp::ReferenceCountedPointer<midp::Displayable>(NULL);
        m_active = false;
    }
}

void MediaPicker::mediaCancel() {
    deactivate();
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->resumeAudio();
}

void MediaPicker::mediaPicked(void* unused) {
    deactivate();
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->resumeAudio();
}

void MediaPicker::activate(midp::Display* display) {
    m_display = display;
    m_previous = display->getCurrent();
    display->setCurrent(this);
    m_active = true;
}

MediaPicker::~MediaPicker() {
    if (m_impl != NULL) {
        m_impl->release();
    }
    midp::DECREF_NO_DELETE(this);
}

MediaPicker::MediaPicker()
    : m_impl(NULL), m_display(NULL), m_previous(NULL), m_active(false), m_unk21(false), m_unk22(false) {
    midp::INCREF(this);
    m_impl = MediaPickerImpl::alloc()->initWithManager(this);
}
