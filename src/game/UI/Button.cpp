#include "game/UI/Button.h"

#include "game/AppEngine.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/WindowStore.h"

static const int TEXT_NULL = 0x800;

namespace UI {

bool Button::pointerPressed(int x, int y, int id) {
    if (m_enabled) {
        m_pressed = true;
    }
    return true;
}

bool Button::pointerReleased(int x, int y, int id) {
    if (m_enabled) {
        WindowStore& store = AppEngine::getCanvas()->getWindowStore();
        int cx = m_x + (m_width >> 1);
        int cy = m_y + (m_height >> 1);
        store.getButtonEffect()->play(cx, cy);
        m_activated = true;
        m_pressed = false;
    }
    return true;
}

Button::Button(int text)
    : m_text(text),
      m_font(8),
      m_unk24(7),
      m_pressed(false),
      m_activated(false),
      m_enabled(true) {}

Button::Button()
    : m_text(TEXT_NULL),
      m_font(-1),
      m_unk24(-1),
      m_pressed(false),
      m_activated(false),
      m_enabled(true) {}

bool Button::isPressed() const {
    return m_pressed;
}

bool Button::isActivated() const {
    return m_activated;
}

void Button::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void Button::unpress() {
    m_pressed = false;
}

void Button::reset() {
    m_activated = false;
}

int Button::getStringId() const {
    return m_text;
}

Button::~Button() {}

}
