#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class PolygonMode : public Object3D {
public:
    PolygonMode();
    virtual ~PolygonMode();
    virtual midp::Class* getClass() const;
    virtual void duplicateTo(Object3D* target);
    virtual int getM3GUniqueClassID() const;
    void setCulling(int mode);
    void setWinding(int mode);
    void setShading(int mode);
    int getCulling() const;
    int getWinding() const;
    int getShading() const;
    void setTwoSidedLightingEnable(bool enable);
    void setLocalCameraLightingEnable(bool enable);
    void setPerspectiveCorrectionEnable(bool enable);
    bool isTwoSidedLightingEnabled() const;
    bool isLocalCameraLightingEnabled() const;
    bool isPerspectiveCorrectionEnabled() const;
    int m_culling;
    int m_winding;
    int m_shading;
    bool m_twoSided;
    bool m_localCamera;
    bool m_perspective;
};

}
