#pragma once

#include "game/GameUI/SolidRect.h"

namespace m3g {
class Graphics3D;
}
namespace midp {
class Graphics;
}

namespace GameUI {

class SoundSlider {
public:
    SoundSlider(int labelId, float maxValue);
    ~SoundSlider();

    float getValue();
    void setValue(float value);
    int getLabelWidth();
    int getHeight();
    void setY(float y);
    bool isPressed();
    void setCoordinates(int labelX, int trackLeft, int trackRight);
    void pointerPressed(int x, int y, int id);
    int pointerDragged(int x, int y, int id);
    bool pointerReleased(int id);
    void render2D(midp::Graphics* g);
    void render3D(m3g::Graphics3D* g3d, bool enabled);

    int m_labelX;
    int m_labelWidth;
    int m_left;
    int m_right;
    int m_y;
    int m_height;
    float m_value;
    float m_max;
    int m_labelId;
    SolidRect m_track;
    int m_pointerId;
};

}
