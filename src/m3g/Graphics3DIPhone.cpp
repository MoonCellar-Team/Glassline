#include "m3g/Graphics3DIPhone.h"
#include "m3g/Renderer.h"
#include "runtime/midp/Graphics.h"

namespace m3g {

Graphics3DIPhone::Graphics3DIPhone() {}

Graphics3DIPhone::~Graphics3DIPhone() {}

void Graphics3DIPhone::bindTarget(midp::Object* target,int hints) {
    Graphics3D::bindTarget(target,hints);
#ifdef MEDECOMP_PORT
    if(target) getRenderer()->bind(medecomp_px(0x300), medecomp_px(0x400 + 2 * medecomp_wide_extra()));
#else
    if(target) getRenderer()->bind(0x300, 0x400);
#endif
}

void Graphics3DIPhone::releaseTarget() {
    midp::Object* target=getTarget();
    if(target) { getRenderer()->release(); static_cast<midp::Graphics*>(target)->bind2D(); }
    Graphics3D::releaseTarget();
}

}
