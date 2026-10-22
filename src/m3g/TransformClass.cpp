#include "m3g/TransformClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

TransformClass::TransformClass() {}

TransformClass::~TransformClass() {}

midp::String* TransformClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Transform");
}

midp::Object* TransformClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Transform); }

}
