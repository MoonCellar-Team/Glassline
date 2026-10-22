#include "runtime/midp/Controllable.h"

namespace midp {

Controllable::Controllable() {}

Controllable::~Controllable() {}

}

bool checkAudioQueueError(long status) {
    return status != 0;
}
