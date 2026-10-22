#include "runtime/midp/ImageClass.h"

#include <stddef.h>
#include "runtime/midp/String.h"

namespace midp {

midp::Object* ImageClass::newInstance() const {
    return NULL;
}

ImageClass::ImageClass() {}

ImageClass::~ImageClass() {}

midp::String* ImageClass::getName() const {
    return new midp::String("javax.microedition.lcdui.Image");
}

}
