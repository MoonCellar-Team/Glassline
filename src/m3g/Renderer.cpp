#include "m3g/Renderer.h"
#include "m3g/RendererClass.h"

namespace m3g {

Renderer::Renderer() {}

Renderer::~Renderer() {}

midp::Class* Renderer::getClass() const { return new RendererClass; }

}
