#include "m3g/Image2DClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

Image2DClass::Image2DClass() {}

Image2DClass::~Image2DClass() {}

midp::String* Image2DClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Image2D");
}

midp::Object* Image2DClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Image2D); }

}
