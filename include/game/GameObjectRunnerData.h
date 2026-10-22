#pragma once

#include "runtime/midp/array.h"

struct GameObjectData_SoloVisual {
    int anim;
    int channel;
    int animFlags;
    int blendFlags;
};

struct GameObjectData_OriginAnimVisual {
    int anim;
    int channel;
    int originAnim;
    bool collide;
    bool useGravity;
    float height;
    int blendFlags;
};

struct GameObjectData_3ChannelBlendVisual {
    int anim;
    int channels[3];
};

class GameObjectRunnerData {
public:
    GameObjectRunnerData();
    virtual ~GameObjectRunnerData();

    void loadData();
    const GameObjectData_SoloVisual& getSoloAnim(int index) const;
    const GameObjectData_OriginAnimVisual& getOriginAnim(int index) const;
    const GameObjectData_3ChannelBlendVisual& get3ChannelBlendAnim(int index) const;

    midp::array<GameObjectData_SoloVisual> m_soloVisuals;
    midp::array<GameObjectData_OriginAnimVisual> m_originVisuals;
    midp::array<GameObjectData_3ChannelBlendVisual> m_blendVisuals;
};
