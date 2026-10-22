#include "game/GameObjectRunnerData.h"

#include "game/AppEngine.h"
#include "game/GameObjectRunner.h"
#include "game/ResourceManager.h"
#include "runtime/midp/DataInputStream.h"

static const int RESOURCE_RUNNER_DATA = 221;

GameObjectRunnerData::GameObjectRunnerData() {}

GameObjectRunnerData::~GameObjectRunnerData() {}

void GameObjectRunnerData::loadData() {
    if (!(m_soloVisuals == NULL)) {
        return;
    }
    AppEngine::getCanvas();
    midp::DataInputStream in(ResourceManager::loadBinaryFile(RESOURCE_RUNNER_DATA));

    int count = in.readUnsignedShort();
    m_soloVisuals = midp::array<GameObjectData_SoloVisual>(count);
    for (int i = 0; i < count; i++) {
        GameObjectData_SoloVisual& v = m_soloVisuals[i];
        v.anim = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.channel = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.animFlags = GameObjectRunner::ANIM_FLAG_ARRAY[in.readUnsignedShort()];
        v.blendFlags = in.readBoolean() ? 4 : 0;
    }

    count = in.readUnsignedShort();
    m_originVisuals = midp::array<GameObjectData_OriginAnimVisual>(count);
    for (int i = 0; i < count; i++) {
        GameObjectData_OriginAnimVisual& v = m_originVisuals[i];
        v.anim = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.channel = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.originAnim = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.collide = in.readBoolean();
        v.useGravity = in.readBoolean();
        v.height = in.readInt() * (1.0f / 65536.0f);
        v.blendFlags = in.readBoolean() ? 4 : 0;
    }

    count = in.readUnsignedShort();
    m_blendVisuals = midp::array<GameObjectData_3ChannelBlendVisual>(count);
    for (int i = 0; i < count; i++) {
        GameObjectData_3ChannelBlendVisual& v = m_blendVisuals[i];
        v.anim = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.channels[0] = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.channels[1] = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
        v.channels[2] = GameObjectRunner::RESOURCE_ARRAY[in.readUnsignedShort()];
    }

    in.close();
}

const GameObjectData_SoloVisual& GameObjectRunnerData::getSoloAnim(int index) const {
    return m_soloVisuals[index];
}

const GameObjectData_OriginAnimVisual& GameObjectRunnerData::getOriginAnim(int index) const {
    return m_originVisuals[index];
}

const GameObjectData_3ChannelBlendVisual& GameObjectRunnerData::get3ChannelBlendAnim(int index) const {
    return m_blendVisuals[index];
}
