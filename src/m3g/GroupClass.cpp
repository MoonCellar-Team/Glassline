#include "m3g/GroupClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

GroupClass::GroupClass() {}

GroupClass::~GroupClass() {}

midp::String* GroupClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Group");
}

midp::Object* GroupClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Group); }

bool GroupClass::_decRef() { return false; }

void GroupClass::_incRef() {}

}
