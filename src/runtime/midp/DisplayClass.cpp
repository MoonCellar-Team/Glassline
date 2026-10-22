#include "runtime/midp/DisplayClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* DisplayClass::newInstance() const {
    return NULL;
}

DisplayClass::DisplayClass() {}

DisplayClass::~DisplayClass() {}

midp::String* DisplayClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Display");
}

}
