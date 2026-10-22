#include "game/AnimationBlender.h"

#include <stddef.h>

#include "game/AnimationBlenderData.h"
#include "game/AppEngine.h"
#include "game/SignalFilter.h"
#include "m3g/AnimationController.h"
#include "m3g/AnimationTrack.h"
#include "m3g/Node.h"
#include "m3g/SkinnedMesh.h"
#include "runtime/util/VectorSE.h"

void AnimationBlender::update(int dt, bool visible) {
    m_time += dt;
    if (!visible) {
        return;
    }
    float total = 0.0f;
    for (int i = 0; i < m_numChannels; i++) {
        m_weightFilters[i]->update(dt);
        total = m_weightFilters[i]->getFilteredValue() + total;
    }
    float scale = 1.0f / total;
    for (int i = 0; i < m_numChannels; i++) {
        m_controllers[i]->setWeight(m_weightFilters[i]->getFilteredValue() * scale);
    }
    if (m_node != NULL) {
        m_node->animate((int)m_time);
    }
}

void AnimationBlender::setAllChannelWeightsToZero(bool immediate) {
    for (int i = 0; i < m_numChannels; i++) {
        m_weights[i] = 0.0f;
        SignalFilter* filter = m_weightFilters[i];
        if (immediate) {
            filter->setSteadyState(m_weights[i]);
        } else {
            filter->setTargetValue(m_weights[i]);
        }
    }
}

void AnimationBlender::setChannelWeight(int channel, float weight, int flags) {
    if (flags & WEIGHT_EXCLUSIVE) {
        setAllChannelWeightsToZero((flags & WEIGHT_IMMEDIATE) != 0);
    }
    if (flags & WEIGHT_RESTART) {
        m_controllers[channel]->setPosition(0.0f, (int)m_time);
    }
    for (int i = 0; i < m_numChannels; i++) {
        if (i != channel) {
            continue;
        }
        m_weights[i] = weight;
        SignalFilter* filter = m_weightFilters[i];
        if (flags & WEIGHT_IMMEDIATE) {
            filter->setSteadyState(m_weights[i]);
        } else {
            filter->setTargetValue(m_weights[i]);
        }
        return;
    }
}

void AnimationBlender::mapAnimationTracks(m3g::Object3D* obj, util::VectorSE<m3g::AnimationTrack>& tracks,
                                          midp::array<m3g::AnimationTrack*>& copies) {
    for (int n = obj->getAnimationTrackCount(); n != 0; n--) {
        m3g::AnimationTrack* track = obj->getAnimationTrack(0);
        obj->removeAnimationTrack(track);
        int index = tracks.indexOf(track);
        if (index != -1) {
            track = copies[index];
        }
        obj->addAnimationTrack(track);
    }
    m3g::SkinnedMesh* mesh = m3g::m3g_cast<m3g::SkinnedMesh>(obj);
    if (mesh != NULL) {
        mapAnimationTracks(mesh->getSkeleton(), tracks, copies);
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(obj);
    if (group != NULL) {
        int count = group->getChildCount();
        for (int i = 0; i != count; i++) {
            mapAnimationTracks(group->getChild(i), tracks, copies);
        }
    }
}

void AnimationBlender::addUniqueAnimationTracksToVector(m3g::Object3D* obj,
                                                        util::VectorSE<m3g::AnimationTrack>& tracks) {
    int count = obj->getAnimationTrackCount();
    for (int i = 0; i != count; i++) {
        m3g::AnimationTrack* track = obj->getAnimationTrack(i);
        if (track != NULL && !tracks.contains(track)) {
            tracks.addElement(track);
        }
    }
    m3g::SkinnedMesh* mesh = m3g::m3g_cast<m3g::SkinnedMesh>(obj);
    if (mesh != NULL) {
        addUniqueAnimationTracksToVector(mesh->getSkeleton(), tracks);
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(obj);
    if (group != NULL) {
        int children = group->getChildCount();
        for (int i = 0; i != children; i++) {
            addUniqueAnimationTracksToVector(group->getChild(i), tracks);
        }
    }
}

void AnimationBlender::setNode(m3g::Node* node) {
    m_node = node;
    midp::array<m3g::AnimationController*> original(m_numChannels);
    for (int i = 0; i != m_numChannels; i++) {
        m3g::AnimationController* controller =
            m3g::m3g_cast<m3g::AnimationController>(m_node->find(m_controllerUserIDs[i]));
        original[i] = controller;
        m_controllers[i] = controller->duplicate();
    }
    util::VectorSE<m3g::AnimationTrack> tracks;
    addUniqueAnimationTracksToVector(node, tracks);
    int count = tracks.size();
    midp::array<m3g::AnimationTrack*> copies(count);
    for (int i = 0; i != count; i++) {
        m3g::AnimationTrack* copy = static_cast<m3g::AnimationTrack*>(tracks.elementAt(i)->duplicate());
        m3g::AnimationController* controller = copy->getController();
        copies[i] = copy;
        for (int j = 0; j != m_numChannels; j++) {
            if (original[j] == controller) {
                copy->setController(m_controllers[j]);
                break;
            }
        }
    }
    mapAnimationTracks(node, tracks, copies);
}

AnimationBlender::~AnimationBlender() {
    for (int i = 0; i < m_numChannels; i++) {
        SignalFilter* filter = m_weightFilters[i];
        if (filter != NULL) {
            delete filter;
            m_weightFilters[i] = NULL;
        }
    }
}

AnimationBlender::AnimationBlender(int type) : m_node(NULL), m_numChannels(0), m_time(0) {
    midp::array<int> userIDs(AppEngine::getAnimationBlenderData()->getAnimationControllerUserIDs(type));
    midp::array<short> interpTimes(AppEngine::getAnimationBlenderData()->getAnimationChannelInterpTimes(type));
    m_numChannels = userIDs.m_length;
    m_controllers = midp::array<m3g::AnimationController*>(m_numChannels);
    m_controllerUserIDs = midp::array<int>(m_numChannels);
    m_weights = midp::array<float>(m_numChannels);
    m_weightFilters = midp::array<SignalFilter*>(m_numChannels);
    for (int i = 0; i < m_numChannels; i++) {
        m_weights[i] = 0.0f;
        m_weightFilters[i] = new SignalFilter(SignalFilter::ONE_POLE_LP, interpTimes[i], i != 0 ? 0.0f : 1.0f);
        m_controllerUserIDs[i] = userIDs[i];
    }
}
