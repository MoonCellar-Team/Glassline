#pragma once

#include "game/UI/WindowElement.h"

namespace UI {

class Button : public WindowElement {
public:
    Button();
    explicit Button(int text);
    virtual ~Button();

    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);

    bool isPressed() const;
    bool isActivated() const;
    void setEnabled(bool enabled);
    void unpress();
    void reset();
    int getStringId() const;

    int m_text;
    int m_font;
    int m_unk24;
    bool m_pressed;
    bool m_activated;
    bool m_enabled;
};

}
