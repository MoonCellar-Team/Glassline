#include "runtime/midp/Display.h"
#include "runtime/midp/Displayable.h"
#include "m3g/Graphics3D.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Graphics3DClass.h"
#include "m3g/Renderer.h"
namespace midp {
class MIDlet;
MIDlet* Graphics3D_getMIDlet();

}
namespace m3g { Graphics3DIPhone* implementation_Graphics3D_getInstance(); Renderer* implementation_Graphics3D_getRendererForTarget(midp::Object*); }

namespace m3g {

Graphics3D::Graphics3D() : m_camera(nullptr),m_scope(-1),m_cameraTransform(),m_viewTransform(),m_projection(),m_target(nullptr),m_depthNear(0.0f),m_depthFar(1.0f),m_hints(0),m_viewportX(0),m_viewportY(0),m_viewportWidth(0),m_viewportHeight(0),m_renderers(11),m_highWater(256),m_worldCameraTransform(),m_renderNodes(),m_transformPool(256),m_transformIndex(0) {
    m_renderNodes.reserve(256);
    for(int i=0;i<256;++i) m_transformPool.get()->addElement(new Transform);
}

Graphics3D::~Graphics3D() { midp::DECREF(m_camera); m_camera=nullptr; }

midp::Class* Graphics3D::getClass() const { return new Graphics3DClass; }

Graphics3D* Graphics3D::getInstance() { return implementation_Graphics3D_getInstance(); }

void Graphics3D::bindTarget(midp::Object* target) { bindTarget(target,0x20); }

void Graphics3D::bindTarget(midp::Object* target,int hints) {
    if(!target) return;
    midp::INCREF(target); midp::DECREF(m_target); m_target=target; m_hints=hints;
    if(m_renderers.get()->get(target)) return;
    Renderer* renderer=implementation_Graphics3D_getRendererForTarget(target); m_renderers.get()->put(target,renderer);
}

void Graphics3D::releaseTarget() { midp::DECREF(m_target); m_target=nullptr; }

midp::Object* Graphics3D::getTarget() const { return m_target; }

Renderer* Graphics3D::getRenderer() const { return m_renderers.get()->get(getTarget()); }

int Graphics3D::getViewportWidth() const { return m_viewportWidth; }

int Graphics3D::getViewportHeight() const { return m_viewportHeight; }

void Graphics3D::clear(const Background* background) {
    if(getViewportWidth()==0 || getViewportHeight()==0) return;
    getRenderer()->clear(background);
}

Camera* Graphics3D::getCamera(Transform* transform) const { if(transform) transform->set(&m_cameraTransform); return m_camera; }

void Graphics3D::setCamera(Camera* camera,const Transform* transform) {
    midp::INCREF(camera); midp::DECREF(m_camera); m_camera=camera;
    if(!transform) { m_cameraTransform.setIdentity(); m_viewTransform.setIdentity(); }
    else { m_cameraTransform.set(transform); m_viewTransform.set(transform); m_viewTransform.invert(); }
    if(!camera) { m_projection.setIdentity(); m_scope=-1; }
    else { camera->getProjection(&m_projection); m_scope=camera->getScope(); }
    getRenderer()->setProjectionAndViewTransform(&m_projection, &m_viewTransform);
}

void Graphics3D::setViewport(int x,int y,int width,int height) {
    m_viewportX=x; m_viewportY=y; m_viewportWidth=width; m_viewportHeight=height;
    midp::Displayable* screen=midp::Display::getDisplay(midp::Graphics3D_getMIDlet())->getCurrent();
#ifdef MEDECOMP_PORT
    const int extra=medecomp_wide_extra();
    if(extra!=0 && x==0 && y==0 && width==screen->getWidth() && height==screen->getHeight()) { x=-extra; width+=2*extra; }
#endif
    int rect[4]={x,y,width,height};
    screen->transformRect(rect);
#ifdef MEDECOMP_PORT
    rect[1]+=extra;
    { int x0=medecomp_px(rect[0]), y0=medecomp_px(rect[1]);
      rect[2]=medecomp_px(rect[0]+rect[2])-x0; rect[3]=medecomp_px(rect[1]+rect[3])-y0; rect[0]=x0; rect[1]=y0; }
#endif
    getRenderer()->setViewport(rect[0], rect[1], rect[2], rect[3]);
}

void Graphics3D::render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,const Transform* transform,int scope,float alpha) const {
    if((m_scope&scope)==0) return;
    Renderer* renderer=getRenderer(); renderer->pushModelTransform(transform);
    renderer->render(vertices, indices, appearance, alpha); renderer->popModelTransform();
}

void Graphics3D::compileRenderableNodeList(const Node* node,const Transform* transform,int scope) const {
    if(!node->isRenderingEnabled()) return;
    const Group* group=m3g_cast<Group>(node);
    if(group) {
        Transform cumulative; int count=group->getChildCount();
        for(int i=0;i<count;++i) {
            Node* child=group->getChild(i); if(!child->isRenderingEnabled()) continue;
            if(!transform) child->getCompositeTransform(&cumulative);
            else { cumulative.set(transform); child->getCompositeTransformCumulative(&cumulative); }
            compileRenderableNodeList(child,&cumulative,scope);
        }
        return;
    }
    const Mesh* mesh=m3g_cast<Mesh>(node); if(!mesh || (mesh->getScope()&scope)==0) return;
    const SkinnedMesh* skinned=m3g_cast<SkinnedMesh>(mesh);
    if(skinned) {
        Transform skeletonTransform; Group* skeleton=skinned->getSkeleton(); skeleton->getCompositeTransform(&skeletonTransform);
        if(!transform) compileRenderableNodeList(skeleton,&skeletonTransform,scope);
        else { Transform combined(transform); combined.postMultiply(&skeletonTransform); compileRenderableNodeList(skeleton,&combined,scope); }
    }
    auto* pool=m_transformPool.get();
    if(pool->size()<=m_transformIndex) pool->addElement(new Transform);
    int index=m_transformIndex++; Transform* copied=static_cast<Transform*>(pool->elementAt(index));
    if(transform) copied->set(transform); else copied->setIdentity();
    int count=mesh->getSubmeshCount();
    for(int i=0;i<count;++i) { RenderNode item(mesh,i,copied,skinned!=nullptr); m_renderNodes.push_back(item); }
}

void Graphics3D::render(const Node* node,const Transform* transform) const {
    m_renderNodes.clear(); m_transformIndex=0; compileRenderableNodeList(node,transform,m_scope);
    std::sort(m_renderNodes.begin(),m_renderNodes.end());
    unsigned count=m_renderNodes.size(); int scope=m_scope; Renderer* renderer=getRenderer();
#ifdef MEDECOMP_PORT
    if(count) medecomp_shadow_pass(&m_renderNodes[0],int(count),renderer);
#endif
    for(unsigned i=0;i<count;++i) {
        RenderNode& item=m_renderNodes[i]; const Mesh* mesh=m3g_cast<Mesh>(item.m_node);
        if(!mesh || !(mesh->getScope()&scope) || !item.m_appearance) continue;
        const VertexBuffer* vertices=item.m_vertices; const IndexBuffer* indices=item.m_indices; const Appearance* appearance=item.m_appearance;
        float alpha=mesh->getAlphaFactor(); renderer->pushModelTransform(item.m_transform);
        const SkinnedMesh* skinned=m3g_cast<SkinnedMesh>(mesh);
        if(!skinned) renderer->render(vertices, indices, appearance, alpha);
        else {
            VertexArray* skinIndices=skinned->getSkinIndices(); VertexArray* weights=skinned->getSkinWeights(); auto* bones=skinned->getBoneTransforms();
            renderer->render(vertices, skinIndices, weights, *bones, indices, appearance, alpha);
        }
        renderer->popModelTransform();
    }
#ifdef MEDECOMP_PORT
    medecomp_shadow_end();
#endif
    count=m_renderNodes.size(); if(m_highWater<count) { m_highWater=count; m_renderNodes.reserve(count); }
    m_renderNodes.clear();
}

void Graphics3D::render(const World* world) {
    if(!world) return; Camera* camera=world->getActiveCamera(); if(!camera) return;
    if(camera->getTransformTo(world,&m_worldCameraTransform)) {
        setCamera(camera,&m_worldCameraTransform);
#ifdef MEDECOMP_PORT
        float jitter[2];
        if(medecomp_taa_jitter(jitter)) {
            float m[16]; m_projection.get(m);
            for(int i=0;i<4;++i) { m[i]+=jitter[0]*m[12+i]; m[4+i]+=jitter[1]*m[12+i]; }
            m_projection.set(m); getRenderer()->setProjectionAndViewTransform(&m_projection,&m_viewTransform);
        }
#endif
        clear(world->getBackground()); render(world,nullptr);
    }
}

}
