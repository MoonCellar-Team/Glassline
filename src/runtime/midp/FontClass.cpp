#include "runtime/midp/FontClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* FontClass::newInstance() const {
    return NULL;
}

FontClass::FontClass() {}

FontClass::~FontClass() {}

midp::String* FontClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Font");
}

}
