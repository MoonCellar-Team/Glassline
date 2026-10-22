#include "m3g/RendererClass.h"
#include "runtime/midp/String.h"

#include "m3g/Placement.h"
namespace m3g {

RendererClass::RendererClass() {}

RendererClass::~RendererClass() {}

midp::String* RendererClass::getName() const { return new midp::String("javax.microedition.m3g.renderer.Renderer"); }

midp::Object* RendererClass::newInstance() const { return nullptr; }

}
