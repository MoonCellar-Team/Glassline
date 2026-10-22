#include "runtime/midp/ControlClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ControlClass::newInstance() const {
    return NULL;
}

ControlClass::ControlClass() {}

ControlClass::~ControlClass() {}

midp::String* ControlClass::getName() const {
    return new midp::String("javax.microedition.media.Control");
}

}
