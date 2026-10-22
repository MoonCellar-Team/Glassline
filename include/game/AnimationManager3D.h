#pragma once

#include "runtime/midp/array.h"

class ResourceManager;

class AnimationManager3D {
public:
    static const int FRAME_TIME = 40;

    AnimationManager3D();
    ~AnimationManager3D();

    int getAnimationDuration(int anim);
    bool loadAnimFile(ResourceManager& resources);
    int getAnimStartFrame(int anim);
    int getAnimEndFrame(int anim);
    int getNumAnimations();

    midp::array<short> m_startFrames;
    midp::array<short> m_endFrames;
    midp::array<midp::array<short> > m_unk28;
    midp::array<midp::array<short> > m_unk3c;
    midp::array<midp::array<signed char> > m_unk50;
    int* m_hermiteTable;
};
