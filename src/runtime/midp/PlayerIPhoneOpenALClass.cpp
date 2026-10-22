#include "runtime/midp/PlayerIPhoneOpenALClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* PlayerIPhoneOpenALClass::newInstance() const {
    return NULL;
}

PlayerIPhoneOpenALClass::PlayerIPhoneOpenALClass() {}

PlayerIPhoneOpenALClass::~PlayerIPhoneOpenALClass() {}

midp::String* PlayerIPhoneOpenALClass::getName() const {
    return new midp::String("javax.microedition.media.iphone.PlayerIPhoneOpenAL");
}

}
