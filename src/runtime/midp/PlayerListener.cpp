#include "runtime/midp/PlayerListener.h"

namespace midp {

const char* const PlayerListener::CLOSED = "closed";
const char* const PlayerListener::END_OF_MEDIA = "endOfMedia";
const char* const PlayerListener::STARTED = "started";
const char* const PlayerListener::STOPPED = "stopped";
const char* const PlayerListener::VOLUME_CHANGED = "volumeChanged";

PlayerListener::PlayerListener() {}

PlayerListener::~PlayerListener() {}

}
