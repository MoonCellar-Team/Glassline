#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class Background : public Object3D {
public:
    Background();
    virtual ~Background();
    int getColor() const;
    bool isColorClearEnabled() const;
    bool isDepthClearEnabled() const;
    void setColor(int color);
    void setColorClearEnable(bool enable);
    void setDepthClearEnable(bool enable);
    virtual int getM3GUniqueClassID() const;
    int m_color;
    bool m_colorClear;
    bool m_depthClear;
};

}
