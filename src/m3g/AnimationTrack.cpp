#include "m3g/AnimationTrack.h"
#include "m3g/AnimationTrackClass.h"
#include "m3g/Object3DFinder.h"
#include "m3g/AnimationController.h"
#include "m3g/KeyframeSequence.h"

namespace m3g {

AnimationTrack::AnimationTrack() : m_sequence(nullptr), m_controller(nullptr), m_property(0), m_sample(), m_emptySample() {}

AnimationTrack::~AnimationTrack() { midp::DECREF(m_sequence); midp::DECREF(m_controller); }

midp::Class* AnimationTrack::getClass() const { return new AnimationTrackClass; }

void AnimationTrack::animateReferences(int) const {}

int AnimationTrack::getM3GUniqueClassID() const { return 2; }

void AnimationTrack::setProperty(int property) { m_property = property; }

int AnimationTrack::getTargetProperty() const { return m_property; }

KeyframeSequence* AnimationTrack::getKeyframeSequence() const { return m_sequence; }

AnimationController* AnimationTrack::getController() const { return m_controller; }

void AnimationTrack::setController(AnimationController* controller) { midp::INCREF(controller); midp::DECREF(m_controller); m_controller = controller; }

void AnimationTrack::setKeyframeSequence(KeyframeSequence* sequence) {
    midp::INCREF(sequence); midp::DECREF(m_sequence); m_sequence = sequence;
    m_sample = midp::array<float>(sequence->getComponentCount());
}

const midp::array<float>& AnimationTrack::getSampleValue(int time) const {
    if (!m_controller) return m_emptySample;
    float position = m_controller->getPosition(time);
    m_sequence->sample(position, 0, m_sample);
    float weight = m_controller->getWeight();
    float* values = static_cast<float*>(m_sample.data());
    for (int i = m_sample.m_length; i > 0; --i, ++values) *values *= weight;
    return m_sample;
}

void AnimationTrack::findReferences(Object3DFinder& finder) const {
    Object3D::findReferences(finder);
    if (finder.getFound()) return;
    finder.find(getKeyframeSequence()); finder.find(getController());
}

int AnimationTrack::getReferences(midp::array<Object3D*>& references) const {
    int count = Object3D::getReferences(references);
    bool controller = m_controller != nullptr;
    bool sequence = m_sequence != nullptr;
    int total = count + sequence + controller;
    if (references != nullptr) {
        if (m_sequence) references[count++] = m_sequence;
        if (m_controller) references[count] = m_controller;
    }
    return total;
}

void AnimationTrack::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target);
    AnimationTrack* result = static_cast<AnimationTrack*>(target);
    result->setProperty(getTargetProperty());
    result->setController(getController());
    result->setKeyframeSequence(getKeyframeSequence());
}

}
