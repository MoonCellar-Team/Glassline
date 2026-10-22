#include "runtime/midp/Player.h"

#include "runtime/midp/Control.h"
#include "runtime/midp/PlayerIPhoneOpenAL.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"

extern "C" {

void* objc_msgSend(void* self, const char* sel, ...);
extern void* OBJC_CLASS_$_PlayerIPhone2;

}

void* operator new(size_t);

namespace midp {

static const int kPrefetched = 300;

static void performOnMain(void* impl, const char* selector) {
    objc_msgSend(impl, "performSelectorOnMainThread:withObject:waitUntilDone:", selector, 0, 1);
}

Control* PlayerIPhoneVideo::getControl(String* type) const {
    INCREF(type);
    DECREF(type);
    return 0;
}

bool PlayerIPhoneVideo::isExclusive() const {
    return true;
}

void PlayerIPhoneVideo::update(bool) {}

long long PlayerIPhoneVideo::getDuration() const {
    return 0;
}

long long PlayerIPhoneVideo::getMediaTime() const {
    return 0;
}

long long PlayerIPhoneVideo::setMediaTime(long long) {
    return 0;
}

void PlayerIPhoneVideo::notifyListeners(const char* event, Object* data) const {
    Player::notifyListeners(event, data);
}

void PlayerIPhoneVideo::stop() {
    if (getState() != STARTED)
        return;
    if (m_impl != 0)
        performOnMain(m_impl, "stop");
    stateTransition(kPrefetched);
}

void PlayerIPhoneVideo::prefetch() {
    Player::prefetch();
    if (getState() != REALIZED)
        return;
    performOnMain(m_impl, "init");
    stateTransition(kPrefetched);
}

void PlayerIPhoneVideo::deallocate() {
    int state = getState();
    if (state == CLOSED)
        return;
    if (state == STARTED)
        stop();
    if (getState() != kPrefetched)
        return;
    if (m_impl != 0) {
        void* impl = m_impl;
        m_impl = 0;
        objc_msgSend(impl, "release");
    }
    stateTransition(REALIZED);
}

void PlayerIPhoneVideo::close() {
    if (getState() == CLOSED)
        return;
    deallocate();
    stateTransition(CLOSED);
}

void PlayerIPhoneVideo::start() {
    Player::start();
    if (getState() == STARTED)
        return;
    stateTransition(STARTED);
    notifyListeners(PlayerListener::STARTED, 0);
    m_waiting = true;
    while (PlayerIPhoneOpenAL::doesSharedContextExist() && getState() == STARTED)
        Thread::yield();
    if (getState() == STARTED)
        performOnMain(m_impl, "play");
    else
        performOnMain(m_impl, "close");
    m_waiting = false;
}

void PlayerIPhoneVideo::realize() {
    if (getState() != UNREALIZED)
        return;
    stateTransition(REALIZED);
    setContentType(new String("video/mp4"));
}

PlayerIPhoneVideo::~PlayerIPhoneVideo() {
    if (m_impl != 0) {
        void* impl = m_impl;
        m_impl = 0;
        objc_msgSend(impl, "release");
    }
}

PlayerIPhoneVideo::PlayerIPhoneVideo(const __CFURL* url) : m_waiting(false), m_impl(0) {
    void* obj = objc_msgSend(OBJC_CLASS_$_PlayerIPhone2, "alloc");
    m_impl = objc_msgSend(obj, "initWithCppInterface::", url, this);
}

Player* PlayerIPhoneVideo::duplicate() {
    return 0;
}

}
