#include "m3g/OpenGLES20Renderer.h"
#include "m3g/RendererSupport.h"
#include "m3g/Appearance.h"
#include "m3g/CompositingMode.h"
#include "m3g/PolygonMode.h"
#include "m3g/Fog.h"
#include "m3g/Background.h"
#include "m3g/CustomShader.h"
#include "m3g/ShaderBinding.h"
#include "runtime/midp/GLES20.h"
#include "runtime/nio/FloatBuffer.h"
namespace midp { void invalidateGraphicsIPhoneShaderState(); }
extern "C" void glClearColor(float,float,float,float);
namespace m3g { int OpenGLES20Renderer::s_texturesToBeDeleted[96]; }

namespace m3g {

void OpenGLES20Renderer::popModelTransform() { m_stack.pop(); }

void OpenGLES20Renderer::pushModelTransform(const Transform* transform) { m_stack.push(); m_stack.top()->postMultiply(transform); }

void OpenGLES20Renderer::setViewport(int x,int y,int width,int height) { m_gl->glViewport(x, m_height-(height+y), width, height); }

void OpenGLES20Renderer::release() {}

bool OpenGLES20Renderer::isSkinning() const { return m_skinning; }

bool OpenGLES20Renderer::isSoftwareSkinning() const { return m_maxBones<m_palette->m_length; }

void OpenGLES20Renderer::renderIndexBuffer(const IndexBuffer* indices) const { drawIndexBuffer(m_gl,indices); }

OpenGLES20Renderer::OpenGLES20Renderer(int textureUnits,midp::GLES20* gl) : m_gl(gl),m_stack(4),m_projectionStack(4),m_textureStacks(nullptr),m_view(),m_transform(),m_textureMatrices(nullptr),m_width(0),m_height(0),m_textureUnits(textureUnits),m_unkE0(false),m_unkE4(0),m_unkE8(0),m_shaderSupport(OpenGLES20ShaderSupport::getInstance()),m_shader(nullptr),m_appearance(nullptr),m_vertices(nullptr),m_fog(nullptr),m_compositing(nullptr),m_polygon(nullptr),m_textures(nullptr),m_skinning(false),m_skinIndices(nullptr),m_skinWeights(nullptr),m_palette(nullptr),m_vbo(false),m_maxBones(0),m_skinBuffer(nullptr) {
    m_skinBuffer=allocateSkinBuffer(4000); m_shaderSupport->bindProgram(0x41012); m_shaderSupport->bindProgram(0x51012); m_shaderSupport->bindProgram(0x43012);
    m_textures=new Texture2D*[textureUnits]; m_textureStacks=new TransformStack[textureUnits]; for(int i=0;i<textureUnits;++i) m_textureStacks[i].setCapacity(2);
    m_textureMatrices=new Transform[textureUnits];
    for(int i=0;i<96;++i) s_texturesToBeDeleted[i]=0;
}

OpenGLES20Renderer::~OpenGLES20Renderer() {
    delete[] m_textures;
    delete[] m_textureMatrices;
    delete[] m_textureStacks; m_textures=nullptr; m_palette=nullptr;
}

void OpenGLES20Renderer::processVertexBufferDefaultColor(const VertexBuffer* vertices,float alpha) const {
    if(!(m_shader->m_flags&0x1000)) return; unsigned color=vertices->getDefaultColor(); const float scale=0.003921568859368563f;
    m_gl->glUniform4f(m_shader->location(2), static_cast<float>((color>>16)&255)*scale, static_cast<float>((color>>8)&255)*scale, static_cast<float>(color&255)*scale, (static_cast<float>(color>>24)*alpha)*scale);
}

void OpenGLES20Renderer::processSkin(const VertexArray* indices,const VertexArray* weights) const {
    if(!indices && !weights) { if(m_maxBones>0) { m_gl->glDisableVertexAttribArray(3); m_gl->glDisableVertexAttribArray(4); } }
    else if(m_maxBones>0) {
        unsigned type=vertexType(indices->getComponentType(),true); int count=indices->getComponentCount(),stride=indices->getDataStride(); const void* data=indices->getData(); glVertexAttribPointer(3,count,type,0,stride,data); m_gl->glEnableVertexAttribArray(3);
        type=vertexType(weights->getComponentType(),true); count=weights->getComponentCount(); stride=weights->getDataStride(); data=weights->getData(); glVertexAttribPointer(4,count,type,1,stride,data); m_gl->glEnableVertexAttribArray(4);
    }
    m_skinIndices=indices; m_skinWeights=weights;
}

void OpenGLES20Renderer::processVertexBufferPositionsSoftwareSkinned(const VertexArray* positions,const float* scaleBias) const {
    softwareSkin(m_skinBuffer,positions,m_skinIndices,m_skinWeights,m_palette,scaleBias,true); m_gl->glEnableVertexAttribArray(0);
    float* data=m_skinBuffer->lockBuffer(); glVertexAttribPointer(0,positions->getComponentCount(),0x1406,0,0,data); m_skinBuffer->unlock();
}

void OpenGLES20Renderer::processVertexBufferPositions(const VertexArray* positions,const float* scaleBias) const {
    if(isSkinning() && isSoftwareSkinning()) { m_gl->glDisableVertexAttribArray(0); processVertexBufferPositionsSoftwareSkinned(positions,scaleBias); return; }
    unsigned type=vertexType(positions->getComponentType()); m_gl->glEnableVertexAttribArray(0); int components=positions->getComponentCount(),stride=positions->getDataStride(); const void* data=vertexPointer(positions,m_vbo); glVertexAttribPointer(0,components,type,0,stride,data);
}

void OpenGLES20Renderer::processVertexBuffer(const VertexBuffer* vertices) const {
    float scaleBias[4]; VertexArray* array=vertices->getPositions(scaleBias); processVertexBufferPositions(array,scaleBias);
    array=vertices->getColors();
    if(!array) m_gl->glDisableVertexAttribArray(2); else { int type=array->getComponentType(); unsigned glType=type==1 ? 0x1401 : (type==3 ? 0x140c : 0x1406); m_gl->glEnableVertexAttribArray(2); int count=array->getComponentCount(),stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glVertexAttribPointer(2,count,glType,glType!=0x1406,stride,data); }
    array=vertices->getNormals();
    if(!array) m_gl->glDisableVertexAttribArray(1); else { unsigned type=vertexType(array->getComponentType()); m_gl->glEnableVertexAttribArray(1); int stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glVertexAttribPointer(1,3,type,type!=0x1406,stride,data); }
    for(int unit=m_textureUnits;unit>0;) { --unit; if(unit>=2) continue; array=vertices->getTexCoords(unit,static_cast<float*>(nullptr));
        if(!array) m_gl->glDisableVertexAttribArray(unit+5); else { unsigned type=vertexType(array->getComponentType()); m_gl->glEnableVertexAttribArray(unit+5); int count=array->getComponentCount(),stride=array->getDataStride(); const void* data=vertexPointer(array,m_vbo); glVertexAttribPointer(unit+5,count,type,type==0x140c,stride,data); }
    }
}

void OpenGLES20Renderer::processVertexBufferPopTexcoordsScaleBias(const VertexBuffer* vertices) const { for(int unit=m_textureUnits;unit>0;) { --unit; if(vertices->getTexCoords(unit,static_cast<float*>(nullptr))) m_textureStacks[unit].pop(); } }

void OpenGLES20Renderer::processVertexBufferPushTexcoordsScaleBias(const VertexBuffer* vertices) const {
    for(int unit=m_textureUnits;unit>0;) { --unit; float scaleBias[4]; if(!vertices->getTexCoords(unit,scaleBias)) continue;
        TransformStack& stack=m_textureStacks[unit]; stack.push(); stack.top()->set(m_textureMatrices+unit); stack.translate(scaleBias[1],scaleBias[2],scaleBias[3]); float scale=scaleBias[0]/1.0f; stack.scale(scale,scale,scale);
        if((static_cast<unsigned>(m_shader->m_flags)>>(4+unit*2))&3) { float matrix[16]; stack.top()->get(matrix); transpose(matrix); m_gl->glUniformMatrix4fv(m_shader->texture(unit)[1], 1, 0, matrix); }
    }
}

void OpenGLES20Renderer::processVertexBufferPositionScaleBias(const VertexBuffer* vertices) const {
    if(!vertices) return; float values[4];
    if(!isSkinning()) { vertices->getPositions(values); m_stack.translate(values[1],values[2],values[3]); m_stack.scale(values[0],values[0],values[0]); }
    else if(!isSoftwareSkinning()) vertices->getPositions(values);
}

void OpenGLES20Renderer::processTexture(Texture2D* texture,int unit) const {
    texture->getCompositeTransform(m_textureMatrices+unit); m_gl->glUniform1i(m_shader->texture(unit)[0], unit);
    Image2D* image=texture->getImage(); if(!image) return;
    int level=texture->getLevelFilter(),filter=texture->getImageFilter(),mag=0x2600,min=0x2600;
    if(filter==0xd2) min=level==0xd1 ? 0x2703 : (level==0xd2 ? 0x2701 : 0x2601);
    else if(filter==0xd1) { mag=0x2601; min=level==0xd1 ? 0x2703 : (level==0xd2 ? 0x2701 : 0x2601); }
    int s=texture->getWrappingS(),t=texture->getWrappingT(); Image2DBindGLES20(m_gl,image,0xde1,mag,min,s==0xf0 ? 0x812f : 0x2901,t==0xf0 ? 0x812f : 0x2901);
}

void OpenGLES20Renderer::setProjectionAndViewTransform(const Transform* projection,const Transform* view) { m_projectionStack.loadAndClear(projection); m_projectionStack.push(); m_view.set(view); m_projectionStack.top()->postMultiply(view); m_stack.loadIdentity(); }

void OpenGLES20Renderer::bind(int width,int height) {
    midp::invalidateGraphicsIPhoneShaderState(); m_width=width; m_height=height; m_appearance=nullptr; m_vertices=nullptr; m_fog=reinterpret_cast<Fog*>(-1); m_compositing=reinterpret_cast<CompositingMode*>(-1); m_polygon=reinterpret_cast<PolygonMode*>(-1);
    for(int i=0;i<m_textureUnits;++i) m_textures[i]=reinterpret_cast<Texture2D*>(-1); m_skinIndices=nullptr; m_skinWeights=nullptr;
    m_gl->glEnable(0xb71); m_gl->glDepthMask(1); m_projectionStack.loadIdentity(); midp::OpenGLESManager::flushDeletedTextures();
}

void OpenGLES20Renderer::clear(const Background* background) {
    unsigned mask=0; const float scale=0.003921568859368563f;
    if(!background) { glClearColor(0,0,0,1.0f); m_gl->glDepthMask(1); mask=0x4100; }
    else { if(background->isColorClearEnabled()) { unsigned color=background->getColor(); mask=0x4000; glClearColor(static_cast<float>((color>>16)&255)*scale,static_cast<float>((color>>8)&255)*scale,static_cast<float>(color&255)*scale,static_cast<float>(color>>24)*scale); } if(background->isDepthClearEnabled()) { mask|=0x100; m_gl->glDepthMask(1); } else if(mask==0) return; }
    glClear(mask);
}

unsigned OpenGLES20Renderer::getShaderType(const VertexBuffer* vertices,const Appearance* appearance) const {
    unsigned flags=vertices->getColors() ? 0x1002 : 0x1000;
    for(int unit=0;unit<2;++unit) { VertexArray* coords=vertices->getTexCoords(unit,static_cast<float*>(nullptr)); if(coords) flags|=(coords->getComponentCount()==3 ? 2u : 1u)<<(4+unit*2); }
    CompositingMode* compositing=appearance->getCompositingMode(); if(compositing && compositing->getAlphaThreshold()>=0.003921568859368563f) flags|=0x2000;
    Fog* fog=appearance->getFog(); if(fog) flags|=fog->getMode()==0x51 ? 0x4000 : 0x8000;
    for(int unit=0;unit<4;++unit) if(flags&(3u<<(4+unit*2))) { Texture2D* texture=appearance->getTexture(unit); if(!texture) flags&=~(3u<<(4+unit*2)); else { unsigned mode=5; switch(texture->getBlending()) { case 0xe0: mode=1; break; case 0xe1: mode=2; break; case 0xe2: mode=3; break; case 0xe3: mode=4; break; } flags|=mode<<(16+unit*3); } }
    return flags;
}

void OpenGLES20Renderer::processAppearance(const Appearance* appearance) const {
    if(!appearance) return;
    CompositingMode* compositing=appearance->getCompositingMode();
    if(compositing && (m_shader->m_flags&0x2000)) { float threshold=compositing->getAlphaThreshold(); if(threshold>=0.003921568859368563f) m_gl->glUniform1f(m_shader->location(3), threshold); }
    if(m_compositing!=compositing) {
        if(!compositing) { m_gl->glDisable(0x8037); m_gl->glEnable(0xb71); m_gl->glDepthMask(1); m_gl->glColorMask(1, 1, 1, 1); m_gl->glDisable(0xbe2); }
        else {
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
        if(!polygon) { m_gl->glEnable(0xb44); m_gl->glCullFace(0x405); m_gl->glFrontFace(0x901); }
        else { int culling=polygon->getCulling(); if(culling==0xa2) m_gl->glDisable(0xb44); else { m_gl->glEnable(0xb44); m_gl->glCullFace(culling==0xa1 ? 0x404 : 0x405); } m_gl->glFrontFace(polygon->getWinding()==0xa9 ? 0x900 : 0x901); }
        m_polygon=polygon;
    }
    Fog* fog=appearance->getFog();
    if(fog && (m_shader->m_flags&0xc000)) {
        if(fog->getMode()==0x51) { int location=m_shader->location(5); float nearDistance=fog->getNearDistance(), farDistance=fog->getFarDistance(); m_gl->glUniform2f(location, nearDistance, farDistance); }
        else { int location=m_shader->location(5); float density=fog->getDensity(); m_gl->glUniform1f(location, density); }
        unsigned color=fog->getColor(); const float scale=0.003921568859368563f; m_gl->glUniform3f(m_shader->location(4), static_cast<float>((color>>16)&255)*scale, static_cast<float>((color>>8)&255)*scale, static_cast<float>(color&255)*scale);
    }
    for(int unit=m_textureUnits;unit>0;) { --unit; if(unit>=2 || !((static_cast<unsigned>(m_shader->m_flags)>>(4+unit*2))&3)) continue;
        Texture2D* texture=appearance->getTexture(unit); m_gl->glActiveTexture(unit+0x84c0); if(texture && m_textures[unit]!=texture) { processTexture(texture,unit); m_textures[unit]=texture; }
    }
}

void OpenGLES20Renderer::render(const VertexBuffer* vertices,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha) {
    const VertexBuffer* previous=m_vertices; const AppearanceBase* previousAppearance=m_appearance;
    CustomShader* custom=const_cast<Appearance*>(static_cast<const Appearance*>(appearance))->getCustomShader();
#ifdef MEDECOMP_PORT
    if(!custom) m_shader=static_cast<ShaderBinding*>(m_shaderSupport->bindProgram(getShaderType(vertices,static_cast<const Appearance*>(appearance))|(medecomp_shadows_active() ? MEDECOMP_SHADOW_FLAG : 0u)));
#else
    if(!custom) m_shader=static_cast<ShaderBinding*>(m_shaderSupport->bindProgram(getShaderType(vertices,static_cast<const Appearance*>(appearance))));
#endif
    else { m_shaderSupport->notifyCacheInvalidation(); m_shader=custom->bind(*m_stack.top()); }
    if(m_shader->m_flags&1) { float matrix[16]; m_stack.top()->get(matrix); transpose(matrix); m_gl->glUniformMatrix4fv(m_shader->location(1), 1, 0, matrix); }
    if(previousAppearance!=appearance) { processAppearance(static_cast<const Appearance*>(appearance)); m_appearance=appearance; }
    m_skinning=false; m_vbo=vertices->getBufferObject()!=0; if(m_vbo) m_gl->glBindBuffer(0x8892, vertices->getBufferObject());
    if(previous!=vertices) { processVertexBuffer(vertices); m_vertices=vertices; }
    processVertexBufferPushTexcoordsScaleBias(vertices); processVertexBufferPositionScaleBias(vertices); processVertexBufferDefaultColor(vertices,alpha);
    Transform combined(m_projectionStack.top()); combined.postMultiply(m_stack.top()); combined.get(m_worldViewProjection); transpose(m_worldViewProjection); m_gl->glUniformMatrix4fv(m_shader->location(0), 1, 0, m_worldViewProjection);
#ifdef MEDECOMP_PORT
    if((m_shader->m_flags&MEDECOMP_SHADOW_FLAG) || custom) medecomp_shadow_receive(m_gl,m_shader->m_program,m_stack.top());
#endif
    if(custom) { custom->setView(m_view); custom->setScaledWorld(*m_stack.top()); }
    renderIndexBuffer(indices); processVertexBufferPopTexcoordsScaleBias(vertices);
    if(m_vbo) m_gl->glBindBuffer(0x8892, 0);
}

void OpenGLES20Renderer::render(const VertexBuffer* vertices,const VertexArray* skinIndices,const VertexArray* skinWeights,const midp::array<Transform*>& palette,const IndexBuffer* indices,const AppearanceBase* appearance,float alpha) {
    const VertexBuffer* previous=m_vertices; const AppearanceBase* previousAppearance=m_appearance;
    CustomShader* custom=const_cast<Appearance*>(static_cast<const Appearance*>(appearance))->getCustomShader();
    if(!custom) m_shader=static_cast<ShaderBinding*>(m_shaderSupport->bindProgram(getShaderType(vertices,static_cast<const Appearance*>(appearance))));
    else { m_shaderSupport->notifyCacheInvalidation(); m_shader=custom->bind(*m_stack.top()); }
    if(m_shader->m_flags&1) { float matrix[16]; m_stack.top()->get(matrix); transpose(matrix); m_gl->glUniformMatrix4fv(m_shader->location(1), 1, 0, matrix); }
    if(previousAppearance!=appearance) { processAppearance(static_cast<const Appearance*>(appearance)); m_appearance=appearance; }
    m_skinning=true;
    bool changed=m_skinIndices!=skinIndices || m_skinWeights!=skinWeights || m_palette!=&palette;
    if(changed) { m_palette=&palette; processSkin(skinIndices,skinWeights); }
    if(changed || previous!=vertices) processVertexBuffer(vertices);
    m_vertices=vertices; processVertexBufferPushTexcoordsScaleBias(vertices); processVertexBufferDefaultColor(vertices,alpha); processVertexBufferPositionScaleBias(vertices);
    Transform combined(m_projectionStack.top()); combined.postMultiply(m_stack.top()); combined.get(m_worldViewProjection); transpose(m_worldViewProjection); m_gl->glUniformMatrix4fv(m_shader->location(0), 1, 0, m_worldViewProjection);
    if(custom) { custom->setView(m_view); custom->setScaledWorld(*m_stack.top()); }
    renderIndexBuffer(indices); processVertexBufferPopTexcoordsScaleBias(vertices);
}

}
