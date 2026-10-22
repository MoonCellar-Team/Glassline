#pragma once
#include "runtime/midp/ReferenceCounted.h"
namespace m3g { class Transform; }
namespace m3g { class ShaderBinding; }

namespace m3g {

class CustomShader : public midp::ReferenceCounted {
public:
    CustomShader();
    virtual ~CustomShader();
    virtual ShaderBinding* bind(const Transform& transform) = 0;
    virtual void setScaledWorld(const Transform& world);
    virtual void setView(const Transform& view);
    void* m_gl;
};

}

void transpose(float* matrix);
