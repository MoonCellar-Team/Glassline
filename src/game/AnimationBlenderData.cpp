#include "game/AnimationBlenderData.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_BLENDER_DATA = 183;

AnimationBlenderData::AnimationBlenderData() : m_loaded(false) {}

midp::array<short>& AnimationBlenderData::getAnimationChannelInterpTimes(int type) {
    return m_interpTimes[type];
}

midp::array<int>& AnimationBlenderData::getAnimationControllerUserIDs(int type) {
    return m_controllerUserIDs[type];
}

AnimationBlenderData::~AnimationBlenderData() {}

void AnimationBlenderData::loadData() {
    if (m_loaded) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_BLENDER_DATA));
    int count = in.readShort();
    m_controllerUserIDs = midp::array<midp::array<int> >(count);
    m_interpTimes = midp::array<midp::array<short> >(count);
    for (int i = 0; i < count; i++) {
        int channels = in.readShort();
        m_controllerUserIDs[i] = midp::array<int>(channels);
        m_interpTimes[i] = midp::array<short>(channels);
        for (int j = 0; j < channels; j++) {
            m_controllerUserIDs[i][j] = ResourceManager::ANIMATION_CONTROLLER_LOOKUP[in.readShort()];
            m_interpTimes[i][j] = in.readShort();
        }
    }
    in.close();
    m_loaded = true;
}
