#pragma once

#include <vector>

#include "game/UI/WindowElement.h"

namespace midp {
class Graphics;
}

namespace UI {

class NotchedSlider : public WindowElement {
public:
    NotchedSlider();
    virtual ~NotchedSlider();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);

    void addItem(WindowElement* item);
    WindowElement* getSelectedItem() const;
    int getSelectedNotch() const;
    virtual void update(int dt);
    virtual bool pointerDragged(int x, int y, int id);

    void setLooped(bool looped);
    void setNotchWidth(int width);
    void setRenderExtra(int extra);
    void setTargetNotch(int notch);
    bool canPrev();
    bool canNext();
    void prev();
    void next();
    bool isDragging() const;
    bool isIdle() const;
    int getItemCount() const;

    std::vector<WindowElement*> m_items;
    float m_halfNotchWidth;
    float m_velocity;
    int m_unk30;
    bool m_dragging;
    WindowElement* m_selected;
    int m_selectedNotch;
    int m_targetNotch;
    int m_notchWidth;
    int m_renderExtra;
    int m_unk4C;
    bool m_looped;
};

}
