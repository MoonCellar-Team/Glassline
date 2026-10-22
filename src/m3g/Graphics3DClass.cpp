#include "m3g/Graphics3DClass.h"
#include "runtime/midp/String.h"

#include "m3g/Placement.h"
namespace m3g {

Graphics3DClass::Graphics3DClass() {}

Graphics3DClass::~Graphics3DClass() {}

midp::String* Graphics3DClass::getName() const { return new midp::String("javax.microedition.m3g.Graphics3D"); }

midp::Object* Graphics3DClass::newInstance() const { return nullptr; }

}
