#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class Fog : public Object3D {
public:
    int getColor() const;
    float getDensity() const;
    float getFarDistance() const;
    int getMode() const;
    float getNearDistance() const;
    int m_mode;
    float m_density;
    float m_nearDistance;
    float m_farDistance;
    int m_color;
};

}
