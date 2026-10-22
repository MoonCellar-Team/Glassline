#include "m3g/AppearanceClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

AppearanceClass::AppearanceClass() {}

AppearanceClass::~AppearanceClass() {}

midp::String* AppearanceClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Appearance");
}

midp::Object* AppearanceClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Appearance); }

}
