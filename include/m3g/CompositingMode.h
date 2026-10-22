#pragma once
#include "m3g/Object3D.h"
namespace m3g { class Blender; }

namespace m3g {

class CompositingMode : public Object3D {
public:
    CompositingMode();
    virtual ~CompositingMode();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    virtual void duplicateTo(Object3D* target);
    void setBlending(int value);
    int getBlending() const;
    void setBlender(Blender* blender);
    Blender* getBlender() const;
    void setAlphaThresholdx(int value);
    int getAlphaThresholdx() const;
    void setAlphaThreshold(float value);
    float getAlphaThreshold() const;
    void setAlphaWriteEnable(bool value);
    void setColorWriteEnable(bool value);
    void setDepthWriteEnable(bool value);
    void setDepthTestEnable(bool value);
    bool isAlphaWriteEnabled() const;
    bool isColorWriteEnabled() const;
    bool isDepthWriteEnabled() const;
    bool isDepthTestEnabled() const;
    void setDepthOffsetx(int factor, int units);
    void setDepthOffset(float factor, float units);
    int getDepthOffsetFactorx() const;
    int getDepthOffsetUnitsx() const;
    float getDepthOffsetFactor() const;
    float getDepthOffsetUnits() const;
    int m_blending;
    Blender* m_blender;
    int m_alphaThreshold;
    bool m_depthTest;
    bool m_depthWrite;
    bool m_colorWrite;
    bool m_alphaWrite;
    int m_depthOffsetFactor;
    int m_depthOffsetUnits;
};

}
