#include "runtime/midp/MIDletClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* MIDletClass::newInstance() const {
    return NULL;
}

MIDletClass::MIDletClass() {}

MIDletClass::~MIDletClass() {}

midp::String* MIDletClass::getName() const {
    return new midp::String("javax.microedition.midlet.MIDlet");
}

}
