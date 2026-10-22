#pragma once
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"
namespace midp { class Class; }
namespace m3g { class Object3DFinder; class AnimationTrack; }

namespace m3g {

class Object3D : public midp::Object {
public:
    Object3D();
    virtual ~Object3D();
    virtual midp::Class* getClass() const;
    virtual void updateAnimationProperty(int property, const midp::array<float>& value);
    virtual void updateAnimationProperty(int property, const midp::array<int>& value);
    virtual void postAnimate(int time);
    virtual void duplicateTo(Object3D* target);
    virtual void findReferences(Object3DFinder& finder) const;
    virtual void animateReferences(int time) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual int getM3GUniqueClassID() const = 0;
    Object3D* find(int userID);
    Object3D* duplicate();
    int animate(int time);
    void setUserID(int userID);
    int getUserID() const;
    int getAnimationTrackCount() const;
    AnimationTrack* getAnimationTrack(int index) const;
    void addAnimationTrack(AnimationTrack* track);
    void removeAnimationTrack(AnimationTrack* track);
    util::VectorSE<midp::Object>* m_tracks;
    int m_trackCount;
    int m_userID;
    midp::Object* m_userObject;
    midp::array<Object3D*> m_references;
};

}
