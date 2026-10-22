#include "m3g/OpenGLES11Renderer.h"
#include "m3g/RendererSupport.h"
#include "m3g/Appearance.h"
#include "m3g/CompositingMode.h"
#include "m3g/PolygonMode.h"
#include "m3g/Fog.h"
#include "m3g/Background.h"
#include "runtime/midp/GLES11.h"
#include "runtime/nio/FloatBuffer.h"

namespace m3g {

OpenGLES11Renderer::OpenGLES11Renderer(int textureUnits,midp::GLES11* gl) : m_gl(gl),m_stack(4),m_unk18(0),m_projection(),m_view(),m_world(),m_texture0(),m_texture1(),m_width(0),m_height(0),m_textureUnits(textureUnits),m_vbo(false),m_appearance(nullptr),m_vertices(nullptr),m_fog(nullptr),m_compositing(nullptr),m_polygon(nullptr),m_textures(nullptr),m_skinning(false),m_skinIndices(nullptr),m_skinWeights(nullptr),m_palette(nullptr),m_maxBones(0),m_skinBuffer(nullptr) {
    m_textures=new Texture2D*[textureUnits]; m_skinBuffer=allocateSkinBuffer(4000);
}

OpenGLES11Renderer::~OpenGLES11Renderer() { delete[] m_textures; m_textures=nullptr; m_palette=nullptr; }

void OpenGLES11Renderer::popModelTransform() { m_stack.pop(); }

void OpenGLES11Renderer::pushModelTransform(const Transform* transform) { m_stack.push(); m_stack.top()->postMultiply(transform); }

void OpenGLES11Renderer::setViewport(int x,int y,int width,int height) { m_gl->glViewport(x, m_height-(height+y), width, height); }

void OpenGLES11Renderer::release() {}

bool OpenGLES11Renderer::isSkinning() const { return m_skinning; }

bool OpenGLES11Renderer::isSoftwareSkinning() const { return m_maxBones<m_palette->m_length; }

void OpenGLES11Renderer::renderIndexBuffer(const IndexBuffer* indices) const { drawIndexBuffer(m_gl,indices); }

void OpenGLES11Renderer::processVertexBufferPositionsSoftwareSkinned(const VertexArray* positions,const float* scaleBias) const {
    softwareSkin(m_skinBuffer,positions,m_skinIndices,m_skinWeights,m_palette,scaleBias,false);
    float* data=m_skinBuffer->lockBuffer(); glVertexPointer(positions->getComponentCount(),0x1406,0,data); m_skinBuffer->unlock();
}

void OpenGLES11Renderer::processVertexBufferPositions(const VertexArray* positions,const float* scaleBias) const {
    if(isSkinning() && isSoftwareSkinning()) { processVertexBufferPositionsSoftwareSkinned(positions,scaleBias); return; }
    unsigned type=vertexType(positions->getComponentType()); int components=positions->getComponentCount(), stride=positions->getDataStride(); const void* data=vertexPointer(positions,m_vbo);
    glVertexPointer(components,type,stride,data);
}

void OpenGLES11Renderer::processVertexBuffer(const VertexBuffer* vertices) const {
    float scaleBias[4]; VertexArray* array=vertices->getPositions(scaleBias); processVertexBufferPositions(array,scaleBias);
    array=vertices->getColors();
    if(!array) m_gl->glDisableClientState(0x8076);
    else { int componentType=array->getComponentType(); unsigned type=componentType==1 ? 0x1401 : (componentType==3 ? 0x140c : 0x1406); m_gl->glEnableClientState(0x8076); int count=array->getComponentCount(),stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glColorPointer(count,type,stride,data); }
    array=vertices->getNormals();
    if(!array) { m_gl->glDisable(0x803a); m_gl->glDisableClientState(0x8075); }
    else { unsigned type=vertexType(array->getComponentType()); m_gl->glEnable(0x803a); m_gl->glEnableClientState(0x8075); int stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glNormalPointer(type,stride,data); }
    for(int unit=m_textureUnits;unit>0;) { --unit; m_gl->glClientActiveTexture(unit+0x84c0); array=vertices->getTexCoords(unit,static_cast<float*>(nullptr));
        if(!array) m_gl->glDisableClientState(0x8078);
        else { unsigned type=vertexType(array->getComponentType()); m_gl->glEnableClientState(0x8078); int count=array->getComponentCount(),stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glTexCoordPointer(count,type,stride,data); }
    }
}

void OpenGLES11Renderer::processSkin(const VertexArray* indices,const VertexArray* weights) const {
    if(!indices && !weights) { if(m_maxBones>0) { m_gl->glDisableClientState(0x86ad); m_gl->glDisableClientState(0x8844); } }
    else if(m_maxBones>0) {
        unsigned type=vertexType(indices->getComponentType(),true); int count=indices->getComponentCount(),stride=indices->getDataStride(); const void* data=indices->getData(); glMatrixIndexPointerOES(count,type,stride,data);
        type=vertexType(weights->getComponentType(),true); count=weights->getComponentCount(); stride=weights->getDataStride(); data=weights->getData(); glWeightPointerOES(count,type,stride,data);
        m_gl->glEnableClientState(0x86ad); m_gl->glEnableClientState(0x8844);
    }
    m_skinIndices=indices; m_skinWeights=weights;
}

void OpenGLES11Renderer::processVertexBufferPopTexcoordsScaleBias(const VertexBuffer* vertices) const {
    for(int unit=m_textureUnits;unit>0;) { --unit; if(vertices->getTexCoords(unit,static_cast<float*>(nullptr))) { m_gl->glActiveTexture(unit+0x84c0); m_gl->glMatrixMode(0x1702); glPopMatrix(); } }
}

void OpenGLES11Renderer::processVertexBufferPushTexcoordsScaleBias(const VertexBuffer* vertices) const {
    for(int unit=m_textureUnits;unit>0;) { --unit; float bias[4]; if(vertices->getTexCoords(unit,bias)) { m_gl->glActiveTexture(unit+0x84c0); m_gl->glMatrixMode(0x1702); glPushMatrix(); glTranslatef(bias[1],bias[2],bias[3]); glScalef(bias[0],bias[0],bias[0]); } }
}

void OpenGLES11Renderer::processVertexBufferPositionScaleBias(const VertexBuffer* vertices) const {
    if(!vertices) return;
    if(!isSkinning()) { float values[4]; vertices->getPositions(values); m_stack.translate(values[1],values[2],values[3]); m_stack.scale(values[0],values[0],values[0]); }
    else if(!isSoftwareSkinning()) { float values[4]; vertices->getPositions(values); uploadTransformPalette(m_palette,values,static_cast<int>(reinterpret_cast<__UINTPTR_TYPE__>(m_gl)),nullptr); }
}

void OpenGLES11Renderer::setProjectionAndViewTransform(const Transform* projection,const Transform* view) {
    float matrix[16]; projection->get(matrix); transpose(matrix); m_gl->glMatrixMode(0x1701); glLoadMatrixf(matrix); m_stack.loadAndClear(view);
}

void OpenGLES11Renderer::processVertexBufferDefaultColor(const VertexBuffer* vertices,float alpha) const {
    unsigned color=vertices->getDefaultColor(); const float scale=0.003921568859368563f;
    glColor4f(static_cast<float>((color>>16)&255)*scale,static_cast<float>((color>>8)&255)*scale,static_cast<float>(color&255)*scale,(static_cast<float>(color>>24)*alpha)*scale);
}

void OpenGLES11Renderer::processTexture(Texture2D* texture,int unit) const {
    Transform& transform=unit==0 ? m_texture0 : m_texture1; texture->getCompositeTransform(&transform); float matrix[16]; transform.get(matrix); m_gl->glMatrixMode(0x1702);
    static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if(__builtin_memcmp(matrix,identity,sizeof(matrix))==0) glLoadIdentity(); else { transpose(matrix); glLoadMatrixf(matrix); }
    Image2D* image=texture->getImage(); if(!image) return;
    int mode=0x1e01; switch(texture->getBlending()) { case 0xe0: mode=0x104; break; case 0xe1: mode=0xbe2; break; case 0xe2: mode=0x2101; break; case 0xe3: mode=0x2100; break; }
    glTexEnvf(0x8500,0x8501,-2.0f); int mag,min; textureFiltering(texture,mag,min);
    int s=texture->getWrappingS(),t=texture->getWrappingT(); Image2DBindGLES(m_gl,image,0xde1,mag,min,s==0xf0 ? 0x812f : 0x2901,t==0xf0 ? 0x812f : 0x2901); glTexEnvx(0x2300,0x2200,mode);
}

void OpenGLES11Renderer::bind(int width,int height) {
    m_width=width; m_height=height; m_appearance=nullptr; m_vertices=nullptr; m_fog=reinterpret_cast<Fog*>(-1); m_compositing=reinterpret_cast<CompositingMode*>(-1); m_polygon=reinterpret_cast<PolygonMode*>(-1);
    for(int i=0;i<m_textureUnits;++i) m_textures[i]=reinterpret_cast<Texture2D*>(-1); m_skinIndices=nullptr; m_skinWeights=nullptr;
    m_gl->glEnable(0xb71); m_gl->glDepthMask(1); m_gl->glDisable(0xb50); m_gl->glDisable(0xb60);
    m_gl->glDisableClientState(0x8075); m_gl->glDisableClientState(0x8076); m_gl->glEnableClientState(0x8074); m_gl->glMatrixMode(0x1701); glLoadIdentity(); midp::OpenGLESManager::flushDeletedTextures();
}

void OpenGLES11Renderer::clear(const Background* background) {
    unsigned mask=0;
    if(!background) { glClearColorx(0,0,0,65536); mask=0x4100; }
    else { if(background->isColorClearEnabled()) { unsigned color=background->getColor(); mask=0x4000; glClearColorx((color&0xff0000)/255,((color&0xff00)<<8)/255,((color&255)<<16)/255,((color>>8)&0xff0000)/255); } if(background->isDepthClearEnabled()) mask|=0x100; else if(mask==0) return; }
    glClear(mask);
}

void OpenGLES11Renderer::render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha) {
    const VertexBuffer* previous=m_vertices; if(m_appearance!=appearance) { processAppearance(static_cast<const Appearance*>(appearance)); m_appearance=appearance; }
    m_skinning=false; int buffer=vertices->getBufferObject(); m_vbo=buffer!=0; if(m_vbo) m_gl->glBindBuffer(0x8892, vertices->getBufferObject());
    if(previous!=vertices) { processVertexBuffer(vertices); m_vertices=vertices; }
    processVertexBufferPushTexcoordsScaleBias(vertices); processVertexBufferPositionScaleBias(vertices); processVertexBufferDefaultColor(vertices,alpha);
    m_gl->glMatrixMode(0x1700); float matrix[16]; m_stack.top()->get(matrix); transpose(matrix); glLoadMatrixf(matrix); renderIndexBuffer(indices); processVertexBufferPopTexcoordsScaleBias(vertices);
    if(m_vbo) m_gl->glBindBuffer(0x8892, 0);
}

void OpenGLES11Renderer::render(const VertexBuffer* vertices,const VertexArray* skinIndices,const VertexArray* skinWeights,const midp::array<Transform*>& palette,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha) {
    const VertexBuffer* previous=m_vertices; if(m_appearance!=appearance) { processAppearance(static_cast<const Appearance*>(appearance)); m_appearance=appearance; }
    m_skinning=true;
    bool changed=m_skinIndices!=skinIndices || m_skinWeights!=skinWeights || m_palette!=&palette;
    if(changed) { m_palette=&palette; processSkin(skinIndices,skinWeights); }
    if(changed || previous!=vertices) processVertexBuffer(vertices);
    m_vertices=vertices; processVertexBufferPushTexcoordsScaleBias(vertices); processVertexBufferDefaultColor(vertices,alpha); processVertexBufferPositionScaleBias(vertices);
    m_gl->glMatrixMode(0x1700); float matrix[16]; m_stack.top()->get(matrix); transpose(matrix); glLoadMatrixf(matrix); renderIndexBuffer(indices); processVertexBufferPopTexcoordsScaleBias(vertices);
}

void OpenGLES11Renderer::processAppearance(const Appearance* appearance) const {
    if(!appearance) return;
    CompositingMode* compositing=appearance->getCompositingMode();
    if(m_compositing!=compositing) {
        if(!compositing) { m_gl->glDisable(0xbc0); m_gl->glDisable(0x8037); m_gl->glEnable(0xb71); m_gl->glDepthMask(1); m_gl->glColorMask(1, 1, 1, 1); m_gl->glDisable(0xbe2); }
        else {
            float threshold=compositing->getAlphaThreshold();
            if(threshold>=0.003921568859368563f) { m_gl->glAlphaFunc(0x206, threshold); m_gl->glEnable(0xbc0); } else m_gl->glDisable(0xbc0);
            bool alpha=compositing->isAlphaWriteEnabled(),color=compositing->isColorWriteEnabled(); m_gl->glColorMask(static_cast<int>(color), static_cast<int>(color), static_cast<int>(color), static_cast<int>(alpha));
            float factor=compositing->getDepthOffsetFactor(), units=compositing->getDepthOffsetUnits();
            if(factor==0.0f && units==0.0f) m_gl->glDisable(0x8037); else { m_gl->glPolygonOffset(factor, units); m_gl->glEnable(0x8037); }
            if(compositing->isDepthTestEnabled()) m_gl->glEnable(0xb71); else m_gl->glDisable(0xb71); m_gl->glDepthMask(static_cast<int>(compositing->isDepthWriteEnabled()));
            int source=0,destination=0; bool blend=true;
            switch(compositing->getBlending()) { case 0x40: source=0x302; destination=0x303; break; case 0x41: source=0x302; destination=1; break; case 0x42: source=0x306; destination=0; break; case 0x43: source=0x306; destination=0x300; break; case 0x45: source=destination=1; break; case 0x47: source=1; destination=0x303; break; default: blend=false; }
            if(blend) { m_gl->glBlendFunc(source, destination); m_gl->glEnable(0xbe2); } else m_gl->glDisable(0xbe2);
        }
        m_compositing=compositing;
    }
    PolygonMode* polygon=appearance->getPolygonMode();
    if(m_polygon!=polygon) {
        if(!polygon) { m_gl->glEnable(0xb44); m_gl->glCullFace(0x405); m_gl->glShadeModel(0x1d01); m_gl->glFrontFace(0x901); }
        else { int culling=polygon->getCulling(); if(culling==0xa2) m_gl->glDisable(0xb44); else { m_gl->glEnable(0xb44); m_gl->glCullFace(culling==0xa1 ? 0x404 : 0x405); } m_gl->glShadeModel(polygon->getShading()==0xa4 ? 0x1d00 : 0x1d01); m_gl->glFrontFace(polygon->getWinding()==0xa9 ? 0x900 : 0x901); }
        m_polygon=polygon;
    }
    Fog* fog=appearance->getFog();
    if(!fog) m_gl->glDisable(0xb60);
    else { if(m_fog!=fog) {
        glFogx(0xb65,fog->getMode()==0x51 ? 0x2601 : 0x800); unsigned color=fog->getColor(); const float scale=0.003921568859368563f;
        float rgba[4]={static_cast<float>((color>>16)&255)*scale,static_cast<float>((color>>8)&255)*scale,static_cast<float>(color&255)*scale,static_cast<float>(color>>24)*scale}; glFogfv(0xb66,rgba);
        glFogf(0xb63,fog->getNearDistance()); glFogf(0xb64,fog->getFarDistance()); glFogf(0xb62,fog->getDensity()); m_fog=fog;
    } m_gl->glEnable(0xb60); }
    for(int unit=m_textureUnits;unit>0;) { --unit; Texture2D* texture=appearance->getTexture(unit); m_gl->glActiveTexture(unit+0x84c0);
        if(!texture) m_gl->glDisable(0xde1); else { if(m_textures[unit]!=texture) { processTexture(texture,unit); m_textures[unit]=texture; } m_gl->glEnable(0xde1); }
    }
}

}
