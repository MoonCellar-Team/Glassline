#include "runtime/midp/PlayerIPhoneCoreAudioClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* PlayerIPhoneCoreAudioClass::newInstance() const {
    return NULL;
}

PlayerIPhoneCoreAudioClass::PlayerIPhoneCoreAudioClass() {}

PlayerIPhoneCoreAudioClass::~PlayerIPhoneCoreAudioClass() {}

midp::String* PlayerIPhoneCoreAudioClass::getName() const {
    return new midp::String("javax.microedition.media.iphone.PlayerIPhoneCoreAudio");
}

}
