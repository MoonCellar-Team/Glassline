#include "m3g/Texture2DClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

Texture2DClass::Texture2DClass() {}

Texture2DClass::~Texture2DClass() {}

midp::String* Texture2DClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Texture2D");
}

midp::Object* Texture2DClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Texture2D); }

}
