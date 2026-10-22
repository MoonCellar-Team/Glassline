#include "game/UI/NetworkWaitEffect.h"

#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/WindowStore.h"

namespace UI {

static const int GROUP_NETWORK_WAIT = 52;
static const int ANIM_NETWORK_WAIT = 17;

bool NetworkWaitEffect::isAnimating() const {
    return m_animating;
}

NetworkWaitEffect::NetworkWaitEffect() : m_x(0), m_y(0), m_animating(false) {}

void NetworkWaitEffect::hardStop() {
    AppEngine::getCanvas()->getQuadManager().setGroupVisible(GROUP_NETWORK_WAIT, false);
    m_animating = false;
}

void NetworkWaitEffect::update(int dt) {
    AppEngine::getCanvas()->getQuadManager().updateAnim(ANIM_NETWORK_WAIT, dt);
}

void NetworkWaitEffect::play(int x, int y) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(GROUP_NETWORK_WAIT, true);
    quads.setGroupPosition(GROUP_NETWORK_WAIT, (float)x, (float)y);
    quads.playAnim(ANIM_NETWORK_WAIT, AnimPlayerQuad::PLAY_LOOP);
    m_x = x;
    m_y = y;
    m_animating = true;
}

void NetworkWaitEffect::stop() {
    AppEngine::getCanvas()->getQuadManager().setGroupVisible(GROUP_NETWORK_WAIT, false);
    AppEngine::getCanvas()->getWindowStore().getButtonEffect()->play(m_x, m_y);
    m_animating = false;
}

}
