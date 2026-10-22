#pragma once

#include "runtime/midp/array.h"

class AnimationBlenderData {
public:
    AnimationBlenderData();
    ~AnimationBlenderData();
    void loadData();
    midp::array<short>& getAnimationChannelInterpTimes(int type);
    midp::array<int>& getAnimationControllerUserIDs(int type);

    midp::array<midp::array<int> > m_controllerUserIDs;
    midp::array<midp::array<short> > m_interpTimes;
    bool m_loaded;
};
