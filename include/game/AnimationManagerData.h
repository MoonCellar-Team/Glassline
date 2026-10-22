#pragma once

#include "runtime/midp/array.h"

class AnimPlayer;
namespace midp {
class Image;
}

class AnimationManagerData {
public:
    static const int NUM_PLAYERS = 48;
    static const int NUM_ANIMS = 3;
    static const int NUM_FRAMES = 14;
    static const int NUM_PRIMITIVES = 48;
    static const int NUM_SUBIMAGES = 12;

    struct Subimage {
        short image;
        short x;
        short y;
        short width;
        short height;
    };

    AnimationManagerData();
    ~AnimationManagerData();

    midp::array<AnimPlayer*> m_players;
    midp::array<signed char> m_colors;
    midp::array<signed char> m_frameCounts;
    midp::array<short> m_firstFrame;
    midp::array<short> m_frameDurations;
    midp::array<signed char> m_primitiveCounts;
    midp::array<int> m_firstPrimitive;
    midp::array<short> m_primitives;
    Subimage m_subimages[NUM_SUBIMAGES];
    midp::array<midp::array<midp::Image*> > m_images;
    int m_imageSet;
};
