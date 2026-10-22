#pragma once
#include "m3g/Node.h"

namespace m3g {

class Camera : public Node {
public:
    Camera();
    virtual ~Camera();
    void setGeneric(const Transform* transform);
    void setParallel(float fovy,float aspect,float nearPlane,float farPlane);
    void setPerspective(float fovy,float aspect,float nearPlane,float farPlane);
    void updateCachedProjectionTransform() const;
    int getProjection(Transform* out) const;
    virtual int getM3GUniqueClassID() const;
    int m_projectionType;
    float m_fovy;
    float m_aspect;
    float m_nearPlane;
    float m_farPlane;
    unsigned char m_unk108[8];
    mutable Transform m_projection;
    mutable bool m_projectionValid;
    unsigned char m_unk161[15];
};

}
