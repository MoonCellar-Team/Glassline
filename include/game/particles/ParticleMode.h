#pragma once

#include "runtime/midp/Object.h"
#include "runtime/midp/ReferenceCountedPointer.h"

namespace midp {
class Class;
class String;
}
namespace m3g {
class AppearanceBase;
class KeyframeSequence;
}

namespace particles {

class ParticleMode : public midp::Object {
public:
    explicit ParticleMode(m3g::AppearanceBase* appearance);
    virtual ~ParticleMode();
    virtual midp::Class* getClass() const;

    void setMeanTimeToLive(float time, float deviation);
    void setAppearance(m3g::AppearanceBase* appearance);
    m3g::AppearanceBase* getAppearance() const;
    void setScale(m3g::KeyframeSequence* sequence);
    void setRotation(m3g::KeyframeSequence* sequence);
    void setCrop(m3g::KeyframeSequence* sequence);
    void setColor(m3g::KeyframeSequence* sequence);
    float getMeanTimeToLive() const;
    float getMeanTimeToLiveDeviation() const;
    m3g::KeyframeSequence* getCrop() const;
    m3g::KeyframeSequence* getColor() const;
    m3g::KeyframeSequence* getScale() const;
    m3g::KeyframeSequence* getRotation() const;

    midp::ReferenceCountedPointer<midp::String> m_unk08;
    midp::ReferenceCountedPointer<midp::String> m_unk0c;
    midp::ReferenceCountedPointer<m3g::AppearanceBase> m_appearance;
    float m_meanTimeToLive;
    float m_meanTimeToLiveDeviation;
    midp::ReferenceCountedPointer<m3g::KeyframeSequence> m_scale;
    midp::ReferenceCountedPointer<m3g::KeyframeSequence> m_rotation;
    midp::ReferenceCountedPointer<m3g::KeyframeSequence> m_color;
    midp::ReferenceCountedPointer<m3g::KeyframeSequence> m_crop;
};

}
