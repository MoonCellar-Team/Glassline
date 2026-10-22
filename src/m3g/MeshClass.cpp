#include "m3g/MeshClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

MeshClass::MeshClass() {}

MeshClass::~MeshClass() {}

midp::String* MeshClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Mesh");
}

midp::Object* MeshClass::newInstance() const { return reinterpret_cast<midp::Object*>(new Mesh); }

}
