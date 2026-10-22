#pragma once

#include "runtime/midp/Object.h"

namespace midp {

class Player;

class PlayerListener : public virtual Object {
public:
    static const char* const CLOSED;
    static const char* const END_OF_MEDIA;
    static const char* const STARTED;
    static const char* const STOPPED;
    static const char* const VOLUME_CHANGED;

    PlayerListener();
    virtual ~PlayerListener();
    virtual void playerUpdate(const Player* player, const char* event, Object* data) = 0;
};

}
