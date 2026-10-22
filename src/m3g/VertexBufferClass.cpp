#include "m3g/VertexBufferClass.h"
#include "runtime/midp/String.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Placement.h"

namespace m3g {

VertexBufferClass::VertexBufferClass() {}

VertexBufferClass::~VertexBufferClass() {}

midp::String* VertexBufferClass::getName() const {
    void* storage = ::operator new(sizeof(midp::String));
    return new (storage) midp::String("javax.microedition.m3g.VertexBuffer");
}

midp::Object* VertexBufferClass::newInstance() const { return reinterpret_cast<midp::Object*>(new VertexBuffer); }

}
