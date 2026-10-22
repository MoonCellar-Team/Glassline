#include "runtime/midp/CanvasClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* CanvasClass::newInstance() const {
    return NULL;
}

CanvasClass::CanvasClass() {}

CanvasClass::~CanvasClass() {}

midp::String* CanvasClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Canvas");
}

}
