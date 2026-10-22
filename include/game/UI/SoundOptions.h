#pragma once

#include "game/UI/HorizontalSliderElement.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class SoundOptions : public TitledWindow {
public:
    SoundOptions();
    virtual ~SoundOptions();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);

    float getMusicLevel() const;
    float getEffectsLevel() const;

    HorizontalSliderElement m_music;
    HorizontalSliderElement m_effects;
};

}
