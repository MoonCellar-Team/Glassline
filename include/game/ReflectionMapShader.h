#pragma once

#include "m3g/CustomShader.h"
#include "m3g/ShaderBinding.h"

namespace m3g {
class Image2D;
class Transform;
}
namespace midp {
class GLES20;
}

class ReflectionMapShader : public m3g::CustomShader {
public:
    ReflectionMapShader();
    virtual ~ReflectionMapShader();

    virtual m3g::ShaderBinding* bind(const m3g::Transform& transform);
    virtual void setScaledWorld(const m3g::Transform& world);
    virtual void setView(const m3g::Transform& view);

    void setReflectionParameters(float scale, float intensity);
    void setCameraPos(float x, float y, float z);
    void setReflectionMap(m3g::Image2D* image);
    int generatePShader(int flags);
    int generateVShader(int flags);

    midp::GLES20* gl() const { return static_cast<midp::GLES20*>(m_gl); }

    m3g::ShaderBinding m_binding;
    int m_samplerLocation;
    int m_worldScaledLocation;
    int m_viewLocation;
    int m_cameraPosLocation;
    int m_parametersLocation;
    float m_cameraPos[3];
    float m_parameters[2];
    m3g::Image2D* m_reflectionMap;
};
