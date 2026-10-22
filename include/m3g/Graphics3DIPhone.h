#pragma once
#include "m3g/Graphics3D.h"

namespace m3g {

class Graphics3DIPhone : public Graphics3D {
public:
    Graphics3DIPhone();
    virtual ~Graphics3DIPhone();
    virtual void releaseTarget();
    virtual void bindTarget(midp::Object* target,int hints);
};

}
