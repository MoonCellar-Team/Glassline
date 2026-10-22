#include "m3g/CompositingModeClass.h"
#include "runtime/midp/String.h"
#include "m3g/CompositingMode.h"

#include "m3g/Placement.h"
namespace m3g {

CompositingModeClass::CompositingModeClass() {}

CompositingModeClass::~CompositingModeClass() {}

midp::String* CompositingModeClass::getName() const { return new midp::String("javax.microedition.m3g.CompositingMode"); }

midp::Object* CompositingModeClass::newInstance() const { return new CompositingMode; }

}
