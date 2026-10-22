#include "m3g/SkinnedMeshClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

SkinnedMeshClass::SkinnedMeshClass() {}

SkinnedMeshClass::~SkinnedMeshClass() {}

midp::String* SkinnedMeshClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.Mesh");
}

midp::Object* SkinnedMeshClass::newInstance() const { return reinterpret_cast<midp::Object*>(new SkinnedMesh); }

}
