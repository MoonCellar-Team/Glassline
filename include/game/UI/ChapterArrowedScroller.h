#pragma once

#include "game/UI/LeftArrowButton.h"
#include "game/UI/NotchedSlider.h"
#include "game/UI/RightArrowButton.h"

namespace midp {
class Graphics;
}

namespace UI {

class ChapterArrowedScroller : public NotchedSlider {
public:
    ChapterArrowedScroller();
    virtual ~ChapterArrowedScroller();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    void set(bool looped, int x, int y, int notchWidth);
    void setLooped(bool looped);
    int getItemCount() const;
    void addItem(WindowElement* item);
    WindowElement* getSelectedItem() const;

    LeftArrowButton m_left;
    RightArrowButton m_right;
};

}
