#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class SignalFilter;
namespace m3g {
class AnimationController;
class AnimationTrack;
class Node;
class Object3D;
}
namespace util {
template <class T>
class VectorSE;
}

class AnimationBlender : public midp::ReferenceCounted {
public:
    static const int WEIGHT_RESTART = 1;
    static const int WEIGHT_EXCLUSIVE = 2;
    static const int WEIGHT_IMMEDIATE = 4;

    explicit AnimationBlender(int type);
    virtual ~AnimationBlender();
    void setNode(m3g::Node* node);
    void setChannelWeight(int channel, float weight, int flags);
    void setAllChannelWeightsToZero(bool immediate);
    void update(int dt, bool visible);
    void mapAnimationTracks(m3g::Object3D* obj, util::VectorSE<m3g::AnimationTrack>& tracks,
                            midp::array<m3g::AnimationTrack*>& copies);
    void addUniqueAnimationTracksToVector(m3g::Object3D* obj, util::VectorSE<m3g::AnimationTrack>& tracks);

    midp::ReferenceCountedPointer<m3g::Node> m_node;
    int m_numChannels;
    midp::array<m3g::AnimationController*> m_controllers;
    midp::array<int> m_controllerUserIDs;
    midp::array<float> m_weights;
    midp::array<SignalFilter*> m_weightFilters;
    long long m_time;
};
