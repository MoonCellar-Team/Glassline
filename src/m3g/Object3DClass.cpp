#include "m3g/Object3DClass.h"
#include "runtime/midp/String.h"

#include "m3g/Placement.h"
namespace m3g {

Object3DClass::Object3DClass() {}

Object3DClass::~Object3DClass() {}

midp::String* Object3DClass::getName() const { return new midp::String("javax.microedition.m3g.Object3D"); }

midp::Object* Object3DClass::newInstance() const { return nullptr; }

}
