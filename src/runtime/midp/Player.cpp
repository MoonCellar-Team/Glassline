#include "runtime/midp/Player.h"

#include "runtime/midp/PlayerClass.h"
#include "runtime/midp/PlayerListener.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"

namespace midp {

int Player::getState() const {
    return m_state;
}

VolumeControl* Player::getVolumeControl() const {
    return NULL;
}

PitchControl* Player::getPitchControl() const {
    return NULL;
}

PositionControl* Player::getPositionControl() const {
    return NULL;
}

Class* Player::getClass() const {
    return new PlayerClass();
}

void Player::stateTransition(int state) {
    m_state = state;
}

void Player::setContentType(String* type) {
    INCREF(type);
    DECREF(m_contentType);
    m_contentType = type;
}

void Player::prefetch() {
    if (m_state == UNREALIZED)
        realize();
}

void Player::setLoopCount(int count) {
    m_loopCount = count;
}

int Player::getLoopCount() const {
    return m_loopCount;
}

void Player::start() {
    if (m_state == UNREALIZED || m_state == REALIZED)
        prefetch();
}

void Player::stop() {
    realize();
    if (getState() != REALIZED)
        stateTransition(REALIZED);
}

void Player::pause() {}

void Player::unpause() {}

void Player::notifyListeners(const char* event, Object* data) const {
    int count = m_listeners.size();
    for (int i = 0; i < count; i++) {
        PlayerListener* listener = m_listeners.elementAt(i);
        if (listener != NULL)
            listener->playerUpdate(this, event, data);
    }
}

void Player::addPlayerListener(PlayerListener* listener) {
    m_listeners.addElement(listener);
}

Player::~Player() {
    setContentType(NULL);
}

Player::Player() : m_state(UNREALIZED), m_loopCount(0), m_contentType(NULL) {}

}
