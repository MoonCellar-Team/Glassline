#include "m3g/Object3D.h"
#include "m3g/Object3DFinder.h"
#include "m3g/Object3DClass.h"
#include "m3g/AnimationTrack.h"
#include "m3g/AnimationController.h"
#include "m3g/Placement.h"

namespace m3g {

Object3D::Object3D() : m_tracks(nullptr), m_trackCount(0), m_userID(0), m_userObject(nullptr), m_references() {}

Object3D::~Object3D() {
    if (m_tracks) {
        util::VectorSE<midp::Object>* tracks = m_tracks;
        m_tracks = nullptr;
        midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(tracks));
    }
}

void Object3D::setUserID(int userID) { m_userID = userID; }

int Object3D::getUserID() const { return m_userID; }

int Object3D::getAnimationTrackCount() const { return m_trackCount; }

midp::Class* Object3D::getClass() const { return new Object3DClass; }

void Object3D::postAnimate(int) {}

void Object3D::updateAnimationProperty(int, const midp::array<float>&) {}

void Object3D::updateAnimationProperty(int, const midp::array<int>&) {}

void Object3D::animateReferences(int) const {}

Object3D* Object3D::duplicate() {
    midp::Class* type = getClass();
    midp::INCREF(type);
    Object3D* result = static_cast<Object3D*>(type->newInstance());
    duplicateTo(result);
    midp::DECREF(type);
    return result;
}

Object3D* Object3D::find(int userID) {
    if (getUserID() == userID) return this;
    Object3DFinder finder(userID);
    findReferences(finder);
    return finder.getFound();
}

void Object3D::findReferences(Object3DFinder& finder) const {
    int count = getAnimationTrackCount();
    while (count > 0) { --count; finder.find(getAnimationTrack(count)); }
}

int Object3D::getReferences(midp::array<Object3D*>& references) const {
    int count = getAnimationTrackCount();
    if (references.m_length != 0) {
        for (int i = 0; i < count; ++i) references[i] = getAnimationTrack(i);
    }
    return count;
}

AnimationTrack* Object3D::getAnimationTrack(int index) const { return static_cast<AnimationTrack*>(m_tracks->elementAt(index)); }

void Object3D::addAnimationTrack(AnimationTrack* track) {
    if (!m_tracks) {
        void* storage = ::operator new(sizeof(util::VectorSE<midp::Object>));
        m_tracks = new (storage) util::VectorSE<midp::Object>;
        midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_tracks));
    }
    m_tracks->addElement(track);
    m_trackCount = m_tracks->size();
}

void Object3D::removeAnimationTrack(AnimationTrack* track) { m_tracks->removeElement(track); m_trackCount = m_tracks->size(); }

void Object3D::duplicateTo(Object3D* target) {
    target->setUserID(getUserID());
    int count = getAnimationTrackCount();
    for (int i = 0; i < count; ++i) target->addAnimationTrack(getAnimationTrack(i));
}

int Object3D::animate(int time) {
    animateReferences(time);
    if (getAnimationTrackCount() != 0) {
        midp::Object** current = m_tracks->begin();
        midp::Object** end = m_tracks->end();
        do {
            AnimationTrack* track = static_cast<AnimationTrack*>(*current);
            if (!track->getController()->isZeroWeight()) {
                int property = track->getTargetProperty();
                const midp::array<float>& value = track->getSampleValue(time);
                updateAnimationProperty(property, value);
            }
            ++current;
        } while (current != end);
        postAnimate(time);
    }
    return 0;
}

}
