#include "runtime/midp/PlayerClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* PlayerClass::newInstance() const {
    return NULL;
}

PlayerClass::PlayerClass() {}

PlayerClass::~PlayerClass() {}

midp::String* PlayerClass::getName() const {
    return new midp::String("javax.microedition.media.Player");
}

}
