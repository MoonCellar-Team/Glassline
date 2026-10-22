#include "runtime/midp/GraphicsClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* GraphicsClass::newInstance() const {
    return NULL;
}

GraphicsClass::GraphicsClass() {}

GraphicsClass::~GraphicsClass() {}

midp::String* GraphicsClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Graphics");
}

}
