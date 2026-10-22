#pragma once
#include "m3g/Renderer.h"
#include "m3g/TransformStack.h"
#include "m3g/NioBuffers.h"
#include "runtime/midp/array.h"
namespace midp { class GLES20; }
namespace m3g { class Background; class VertexBuffer; class IndexBuffer; class AppearanceBase; class VertexArray; class Texture2D; class Appearance; class Fog; class CompositingMode; class PolygonMode; class OpenGLES20ShaderSupport; class ShaderBinding; }

namespace m3g {

class OpenGLES20Renderer : public Renderer {
public:
    OpenGLES20Renderer(int textureUnits,midp::GLES20* gl);
    virtual ~OpenGLES20Renderer();
    virtual void setProjectionAndViewTransform(const Transform* projection,const Transform* view);
    virtual void pushModelTransform(const Transform* transform);
    virtual void popModelTransform();
    virtual void setViewport(int x,int y,int width,int height);
    virtual void clear(const Background* background);
    virtual void bind(int width,int height);
    virtual void release();
    virtual void render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha);
    virtual void render(const VertexBuffer* vertices,const VertexArray* skinIndices,const VertexArray* skinWeights,const midp::array<Transform*>& palette,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha);
    unsigned getShaderType(const VertexBuffer* vertices,const Appearance* appearance) const;
    static int s_texturesToBeDeleted[96];
    bool isSkinning() const;
    bool isSoftwareSkinning() const;
    void renderIndexBuffer(const IndexBuffer* indices) const;
    void processVertexBufferPositionsSoftwareSkinned(const VertexArray* positions,const float* scaleBias) const;
    void processVertexBufferPositions(const VertexArray* positions,const float* scaleBias) const;
    void processVertexBuffer(const VertexBuffer* vertices) const;
    void processSkin(const VertexArray* indices,const VertexArray* weights) const;
    void processVertexBufferPopTexcoordsScaleBias(const VertexBuffer* vertices) const;
    void processVertexBufferPushTexcoordsScaleBias(const VertexBuffer* vertices) const;
    void processVertexBufferPositionScaleBias(const VertexBuffer* vertices) const;
    void processVertexBufferDefaultColor(const VertexBuffer* vertices,float alpha) const;
    void processTexture(Texture2D* texture,int unit) const;
    void processAppearance(const Appearance* appearance) const;
    midp::GLES20* m_gl;
    mutable TransformStack m_stack;
    mutable TransformStack m_projectionStack;
    TransformStack* m_textureStacks;
    unsigned char m_unk28[8];
    Transform m_view;
    Transform m_transform;
    Transform* m_textureMatrices;
    int m_width;
    int m_height;
    int m_textureUnits;
    bool m_unkE0;
    int m_unkE4;
    int m_unkE8;
    float m_worldViewProjection[16];
    OpenGLES20ShaderSupport* m_shaderSupport;
    ShaderBinding* m_shader;
    const AppearanceBase* m_appearance;
    const VertexBuffer* m_vertices;
    mutable Fog* m_fog;
    mutable CompositingMode* m_compositing;
    mutable PolygonMode* m_polygon;
    Texture2D** m_textures;
    bool m_skinning;
    mutable const VertexArray* m_skinIndices;
    mutable const VertexArray* m_skinWeights;
    const midp::array<Transform*>* m_palette;
    bool m_vbo;
    int m_maxBones;
    mutable nio::FloatBuffer* m_skinBuffer;
};

}
