#pragma once
#include "m3g/Object3D.h"
namespace m3g { class KeyframeSequence; class AnimationController; }

namespace m3g {

class AnimationTrack : public Object3D {
public:
    AnimationTrack();
    virtual ~AnimationTrack();
    virtual midp::Class* getClass() const;
    virtual void animateReferences(int time) const;
    void setProperty(int property);
    void setController(AnimationController* controller);
    const midp::array<float>& getSampleValue(int time) const;
    void setKeyframeSequence(KeyframeSequence* sequence);
    virtual void findReferences(Object3DFinder& finder) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateTo(Object3D* target);
    virtual int getM3GUniqueClassID() const;
    AnimationController* getController() const;
    KeyframeSequence* getKeyframeSequence() const;
    int getTargetProperty() const;
    KeyframeSequence* m_sequence;
    AnimationController* m_controller;
    int m_property;
    mutable midp::array<float> m_sample;
    midp::array<float> m_emptySample;
};

}
