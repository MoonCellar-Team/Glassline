#pragma once
#include "runtime/midp/array.h"
#include "m3g/Renderer.h"
#include "m3g/TransformStack.h"
#include "m3g/NioBuffers.h"
namespace midp { class GLES11; }
namespace m3g { class Background; class VertexBuffer; class IndexBuffer; class AppearanceBase; class VertexArray; class Texture2D; class Appearance; class Fog; class CompositingMode; class PolygonMode; }

namespace m3g {

class OpenGLES11Renderer : public Renderer {
public:
    OpenGLES11Renderer(int textureUnits,midp::GLES11* gl);
    virtual ~OpenGLES11Renderer();
    virtual void setProjectionAndViewTransform(const Transform* projection,const Transform* view);
    virtual void pushModelTransform(const Transform* transform);
    virtual void popModelTransform();
    virtual void setViewport(int x,int y,int width,int height);
    virtual void clear(const Background* background);
    virtual void bind(int width,int height);
    virtual void release();
    virtual void render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha);
    virtual void render(const VertexBuffer* vertices,const VertexArray* skinIndices,const VertexArray* skinWeights,const midp::array<Transform*>& palette,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha);
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
    midp::GLES11* m_gl;
    mutable TransformStack m_stack;
    int m_unk18;
    unsigned char m_unk1c[4];
    Transform m_projection;
    Transform m_view;
    Transform m_world;
    mutable Transform m_texture0;
    mutable Transform m_texture1;
    int m_width;
    int m_height;
    int m_textureUnits;
    bool m_vbo;
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
    int m_maxBones;
    mutable nio::FloatBuffer* m_skinBuffer;
};

}
