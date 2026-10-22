#include "m3g/PolygonModeClass.h"
#include "runtime/midp/String.h"
#include "m3g/PolygonMode.h"

#include "m3g/Placement.h"
namespace m3g {

PolygonModeClass::PolygonModeClass() {}

PolygonModeClass::~PolygonModeClass() {}

midp::String* PolygonModeClass::getName() const { return new midp::String("javax.microedition.m3g.PolygonMode"); }

midp::Object* PolygonModeClass::newInstance() const { return new PolygonMode; }

}
