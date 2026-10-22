#include "m3g/Transformable.h"
#include "runtime/midp/Math.h"

namespace m3g {

Transformable::Transformable() : m_translation{0,0,0}, m_scale{1,1,1}, m_orientation{0,0,0,65536},
    m_animatedTranslation{0,0,0}, m_animatedScale{0,0,0}, m_animatedOrientation{0,0,0,0},
    m_translationAnimated(false), m_scaleAnimated(false), m_orientationAnimated(false), m_transform(nullptr),
    m_cachedTransform(), m_cacheValid(false) {}

Transformable::~Transformable() { midp::DECREF(m_transform); m_transform = nullptr; }

void Transformable::getTranslation(midp::array<float>& out) const { getTranslation(&out[0]); }

void Transformable::translate(float x, float y, float z) {
    if (x == 0.0f && y == 0.0f && z == 0.0f) return;
    m_translation[0] += x; m_translation[1] += y; m_translation[2] += z; m_cacheValid = false;
}

void Transformable::updateCachedTransform() const {
    if (m_cacheValid) return;
    m_cachedTransform.setIdentity();
    unsigned bits[3]; __builtin_memcpy(bits, m_translation, sizeof(bits));
    if (bits[0] || bits[1] || bits[2]) m_cachedTransform.postTranslate(m_translation[0],m_translation[1],m_translation[2]);
    if (m_orientation[3] != 65536 || m_orientation[0] || m_orientation[1] || m_orientation[2])
        m_cachedTransform.postRotateQuatx(m_orientation[0],m_orientation[1],m_orientation[2],m_orientation[3]);
    if (m_scale[0] != 1.0f || m_scale[1] != 1.0f || m_scale[2] != 1.0f) m_cachedTransform.postScale(m_scale[0],m_scale[1],m_scale[2]);
    if (m_transform) m_cachedTransform.postMultiply(m_transform);
    m_cacheValid = true;
}

void Transformable::getCompositeTransformCumulative(Transform* out) const { updateCachedTransform(); out->postMultiply(&m_cachedTransform); }

void Transformable::getCompositeTransform(Transform* out) const { updateCachedTransform(); out->set(&m_cachedTransform); }

void Transformable::setOrientationQuatx(int x, int y, int z, int w) {
    float fx = static_cast<float>(x)*0.0000152587890625f, fy = static_cast<float>(y)*0.0000152587890625f;
    float fz = static_cast<float>(z)*0.0000152587890625f, fw = static_cast<float>(w)*0.0000152587890625f;
    float length = midp::Math::sqrt(((fx*fx + fy*fy) + fz*fz) + fw*fw);
    long long scale = static_cast<long long>(65536.0f / length);
    int q[4]; const int input[4] = {x,y,z,w};
    for (int i = 0; i < 4; ++i) {
        unsigned long long product = static_cast<unsigned long long>(scale) * static_cast<unsigned long long>(static_cast<long long>(input[i]));
        q[i] = static_cast<int>(product >> 16);
    }
    if (m_orientation[0] == q[0] && m_orientation[1] == q[1] && m_orientation[2] == q[2] && m_orientation[3] == q[3]) return;
    for (int i = 0; i < 4; ++i) m_orientation[i] = q[i];
    m_cacheValid = false;
}

void Transformable::setOrientationQuat(float x, float y, float z, float w) {
    setOrientationQuatx(static_cast<int>(x*65536.0f+0.5f),static_cast<int>(y*65536.0f+0.5f),static_cast<int>(z*65536.0f+0.5f),static_cast<int>(w*65536.0f+0.5f));
}

void Transformable::setOrientationx(int angle, int x, int y, int z) { int q[4]; Transform::angleAxisToQuatx(angle,x,y,z,q); setOrientationQuatx(q[0],q[1],q[2],q[3]); }

void Transformable::setOrientation(float angle, float x, float y, float z) {
    setOrientationx(static_cast<int>(angle*65536.0f+0.5f),static_cast<int>(x*65536.0f+0.5f),static_cast<int>(y*65536.0f+0.5f),static_cast<int>(z*65536.0f+0.5f));
}

void Transformable::setScalex(int x, int y, int z) { setScale(static_cast<float>(x)*0.0000152587890625f,static_cast<float>(y)*0.0000152587890625f,static_cast<float>(z)*0.0000152587890625f); }

void Transformable::setTransform(const Transform* transform) {
    if (!transform) { midp::DECREF(m_transform); m_transform = nullptr; }
    else if (!m_transform) { m_transform = new Transform(transform); midp::INCREF(m_transform); }
    else m_transform->set(transform);
    m_cacheValid = false;
}

void Transformable::postAnimate(int time) {
    Object3D::postAnimate(time);
    if (m_translationAnimated) setTranslation(m_animatedTranslation[0],m_animatedTranslation[1],m_animatedTranslation[2]);
    if (m_scaleAnimated) setScale(m_animatedScale[0],m_animatedScale[1],m_animatedScale[2]);
    if (m_orientationAnimated) {
        float x=m_animatedOrientation[0], y=m_animatedOrientation[1], z=m_animatedOrientation[2], w=m_animatedOrientation[3];
        if (((x*x+y*y)+z*z)+w*w != 0.0f) setOrientationQuat(x,y,z,w);
    }
    for (int i=0;i<3;++i) { m_animatedTranslation[i]=0.0f; m_animatedScale[i]=0.0f; }
    for (int i=0;i<4;++i) m_animatedOrientation[i]=0.0f;
    m_translationAnimated=false; m_scaleAnimated=false; m_orientationAnimated=false;
}

void Transformable::updateAnimationProperty(int property, const midp::array<float>& value) {
    Object3D::updateAnimationProperty(property,value);
    const float* v=&value[0];
    if (property == 0x10c) {
        m_orientationAnimated=true;
        float* q=m_animatedOrientation;
        float dot=((q[0]*v[0]+q[1]*v[1])+q[2]*v[2])+q[3]*v[3];
        if (dot >= 0.0f) for(int i=0;i<4;++i) q[i]+=v[i];
        else for(int i=0;i<4;++i) q[i]-=v[i];
    } else if (property == 0x10e) {
        m_scaleAnimated=true; for(int i=0;i<3;++i) m_animatedScale[i]+=v[i];
    } else if (property == 0x113) {
        m_translationAnimated=true; for(int i=0;i<3;++i) m_animatedTranslation[i]+=v[i];
    }
}

void Transformable::updateAnimationProperty(int property, const midp::array<int>& value) { Object3D::updateAnimationProperty(property,value); }

void Transformable::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target); Transformable* result=static_cast<Transformable*>(target);
    float translation[3]; getTranslation(translation); result->setTranslation(translation[0],translation[1],translation[2]);
    float scale[3]; getScale(scale); result->setScale(scale[0],scale[1],scale[2]);
    int orientation[4]; getOrientationQuatx(orientation); result->setOrientationQuatx(orientation[0],orientation[1],orientation[2],orientation[3]);
    result->setTransform(m_transform);
}

void Transformable::getTranslation(float* out) const { for(int i=0;i<3;++i) out[i]=m_translation[i]; }

void Transformable::getScale(float* out) const { for(int i=0;i<3;++i) out[i]=m_scale[i]; }

void Transformable::getOrientationQuatx(int* out) const { for(int i=0;i<4;++i) out[i]=m_orientation[i]; }

void Transformable::setScale(float x, float y, float z) {
    if (m_scale[0]==x && m_scale[1]==y && m_scale[2]==z) return;
    m_scale[0]=x; m_scale[1]=y; m_scale[2]=z; m_cacheValid=false;
}

void Transformable::setTranslation(float x, float y, float z) {
    if (m_translation[0]==x && m_translation[1]==y && m_translation[2]==z) return;
    m_translation[0]=x; m_translation[1]=y; m_translation[2]=z; m_cacheValid=false;
}

}
