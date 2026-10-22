#include "game/AnimationManagerData.h"

#include <stddef.h>

#include "game/AnimPlayer.h"
#include "runtime/midp/Image.h"

AnimationManagerData::~AnimationManagerData() {
    for (int i = 0; i < NUM_PLAYERS; i++) {
        delete m_players[i];
        m_players[i] = NULL;
    }
    for (int i = 0; i < m_images[0].m_length; i++) {
        midp::Image* image = m_images[0][i];
        m_images[0][i] = NULL;
        midp::DECREF(image);
    }
}

AnimationManagerData::AnimationManagerData() : m_players(NUM_PLAYERS), m_imageSet(0) {}
