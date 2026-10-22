#include "game/UI/ButtonEffect.h"

#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/QuadManager.h"

namespace UI {

static const int GROUP_BUTTON_EFFECT = 51;
static const int ANIM_BUTTON_EFFECT = 16;

ButtonEffect::ButtonEffect() : m_animating(false) {}

void ButtonEffect::update(int dt) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    if (!m_animating) {
        return;
    }
    if (quads.isAnimating(ANIM_BUTTON_EFFECT)) {
        quads.updateAnim(ANIM_BUTTON_EFFECT, dt);
        quads.setGroupVisible(GROUP_BUTTON_EFFECT, true);
    } else if (m_animating) {
        quads.setGroupVisible(GROUP_BUTTON_EFFECT, false);
        m_animating = false;
    }
}

void ButtonEffect::play(int x, int y) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupPosition(GROUP_BUTTON_EFFECT, (float)x, (float)y);
    quads.playAnim(ANIM_BUTTON_EFFECT, AnimPlayerQuad::PLAY_RESTART);
    m_animating = true;
}

bool ButtonEffect::isAnimating() const {
    return m_animating;
}

}
