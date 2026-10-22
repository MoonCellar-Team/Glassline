#pragma once
#include "m3g/Object3D.h"
#include "m3g/Transform.h"

namespace m3g {

class Transformable : public Object3D {
public:
    Transformable();
    virtual ~Transformable();
    void getTranslation(float* out) const;
    void getTranslation(midp::array<float>& out) const;
    void getScale(float* out) const;
    void getOrientationQuatx(int* out) const;
    void setScale(float x, float y, float z);
    void setScalex(int x, int y, int z);
    void setTranslation(float x, float y, float z);
    void translate(float x, float y, float z);
    void updateCachedTransform() const;
    void getCompositeTransformCumulative(Transform* out) const;
    void getCompositeTransform(Transform* out) const;
    void setOrientationQuatx(int x, int y, int z, int w);
    void setOrientationQuat(float x, float y, float z, float w);
    void setOrientationx(int angle, int x, int y, int z);
    void setOrientation(float angle, float x, float y, float z);
    void setTransform(const Transform* transform);
    virtual void postAnimate(int time);
    virtual void updateAnimationProperty(int property, const midp::array<float>& value);
    virtual void updateAnimationProperty(int property, const midp::array<int>& value);
    virtual void duplicateTo(Object3D* target);
    float m_translation[3];
    float m_scale[3];
    int m_orientation[4];
    float m_animatedTranslation[3];
    float m_animatedScale[3];
    float m_animatedOrientation[4];
    bool m_translationAnimated;
    bool m_scaleAnimated;
    bool m_orientationAnimated;
    Transform* m_transform;
    mutable Transform m_cachedTransform;
    mutable bool m_cacheValid;
};

}
