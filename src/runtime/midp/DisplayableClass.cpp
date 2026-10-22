#include "runtime/midp/DisplayableClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* DisplayableClass::newInstance() const {
    return NULL;
}

DisplayableClass::DisplayableClass() {}

DisplayableClass::~DisplayableClass() {}

midp::String* DisplayableClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Displayable");
}

}
