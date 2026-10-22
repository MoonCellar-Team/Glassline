#pragma once
#include "m3g/Transform.h"
#include "m3g/RenderNode.h"
#include "m3g/RendererTable.h"
#include "m3g/OpaqueObject.h"
#include "runtime/util/VectorSE.h"
#include "sdk/vector.h"
namespace m3g {
class Camera; class Renderer; class Background; class World; class Node; class VertexBuffer; class IndexBuffer; class AppearanceBase;
extern bool g_usingGLES20;
using RendererTableStorage=OpaqueObject<util::HashtableSE<midp::Object,Renderer>,0x4c>;
using TransformPoolStorage=OpaqueObject<util::VectorSE<midp::Object>,0x18>;
}

namespace m3g {

class Graphics3D : public midp::Object {
public:
    Graphics3D();
    virtual ~Graphics3D();
    virtual midp::Class* getClass() const;
    static Graphics3D* getInstance();
    void bindTarget(midp::Object* target);
    virtual void bindTarget(midp::Object* target,int hints);
    virtual void releaseTarget();
    midp::Object* getTarget() const;
    Renderer* getRenderer() const;
    virtual void setViewport(int x,int y,int width,int height);
    int getViewportWidth() const;
    int getViewportHeight() const;
    Camera* getCamera(Transform* transform) const;
    virtual void setCamera(Camera* camera,const Transform* transform);
    virtual void clear(const Background* background);
    void render(const World* world);
    void render(const Node* node,const Transform* transform) const;
    void render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,const Transform* transform,int scope,float alpha) const;
    void compileRenderableNodeList(const Node* node,const Transform* transform,int scope) const;
    Camera* m_camera;
    int m_scope;
    Transform m_cameraTransform;
    Transform m_viewTransform;
    Transform m_projection;
    midp::Object* m_target;
    float m_depthNear;
    float m_depthFar;
    int m_hints;
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;
    int m_viewportHeight;
    mutable RendererTableStorage m_renderers;
    mutable unsigned m_highWater;
    Transform m_worldCameraTransform;
    mutable std::vector<RenderNode> m_renderNodes;
    mutable TransformPoolStorage m_transformPool;
    mutable int m_transformIndex;
    unsigned char m_unk1e8[8];
};

}
