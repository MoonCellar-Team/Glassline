#include "m3g/VertexBuffer.h"
#include "m3g/VertexArrayTextureUnit.h"
#include "m3g/VertexBufferClass.h"
#include "m3g/GLHelpers.h"
#include "runtime/midp/GLES11.h"
#include "m3g/VertexHelpers.h"
void* operator new[](__SIZE_TYPE__);
void operator delete[](void*) noexcept;
namespace m3g { int alignOffset(int); VertexArray* createSharedVertexArray(const VertexArray*,void*,int,int); }

namespace m3g {

VertexBuffer::VertexBuffer() : m_vertexCount(0),m_defaultColor(-1),m_positions(nullptr),m_positionScale(1.0f),m_positionBias{0,0,0},
    m_colors(nullptr),m_normals(nullptr),m_pointSizes(nullptr),m_texCoords(2),m_skinIndices(nullptr),m_skinWeights(nullptr),m_bufferObject(0),m_mutable(true),m_sharedData(nullptr) {}

VertexBuffer::~VertexBuffer() {
    midp::DECREF(m_positions); midp::DECREF(m_normals); midp::DECREF(m_colors); midp::DECREF(m_pointSizes);
    for(int i=0;i<m_texCoords.m_length;++i) { VertexArrayTextureUnit* unit=m_texCoords[i]; m_texCoords[i]=nullptr; delete unit; }
    midp::DECREF(m_skinIndices); midp::DECREF(m_skinWeights);
    if(m_sharedData) ::operator delete[](m_sharedData);
    if(m_bufferObject) { midp::OpenGLESManager::removeBuffer(m_bufferObject); m_bufferObject=0; }
}

midp::Class* VertexBuffer::getClass() const { return new VertexBufferClass; }

int VertexBuffer::getM3GUniqueClassID() const { return 21; }

bool VertexBuffer::verifyMutable() const { return m_mutable!=false; }

void VertexBuffer::setDefaultColor(int color) { m_defaultColor=color; }

VertexArray* VertexBuffer::getPositions(float* scaleBias) const {
    if(scaleBias) { scaleBias[0]=m_positionScale; scaleBias[1]=m_positionBias[0]; scaleBias[2]=m_positionBias[1]; scaleBias[3]=m_positionBias[2]; }
    return m_positions;
}

VertexArray* VertexBuffer::getPositions(midp::array<float>& scaleBias) const { return getPositions(&scaleBias[0]); }

void VertexBuffer::setPositions(VertexArray* array,float scale,const float* bias) {
    if(!verifyMutable()) return;
    midp::INCREF(array); midp::DECREF(m_positions); m_positions=array; m_positionScale=scale;
    m_positionBias[0]=bias ? bias[0] : 0.0f; m_positionBias[1]=bias ? bias[1] : 0.0f; m_positionBias[2]=bias ? bias[2] : 0.0f;
    updateVertexCount();
}

void VertexBuffer::setPositions(VertexArray* array,float scale,midp::array<float> bias) { const float* data=bias!=nullptr ? &bias[0] : nullptr; setPositions(array,scale,data); }

void VertexBuffer::updateVertexCount() {
    if(m_positions) { m_vertexCount=m_positions->getVertexCount(); return; }
    VertexArray* array=m_colors ? m_colors : m_normals;
    if(array) { m_vertexCount=array->getVertexCount(); return; }
    VertexArrayTextureUnit* unit=m_texCoords[0]; if(!unit) unit=m_texCoords[1];
    m_vertexCount=unit ? unit->getVertexCount() : 0;
}

VertexArray* VertexBuffer::getTexCoords(int unit,float* scaleBias) const {
    if(!verifyTextureUnit(unit)) return nullptr;
    VertexArrayTextureUnit* coords=m_texCoords[unit]; if(!coords) return nullptr;
    if(scaleBias) { scaleBias[0]=coords->m_scale; scaleBias[1]=coords->m_bias[0]; scaleBias[2]=coords->m_bias[1]; scaleBias[3]=0.0f; }
    return coords->m_array;
}

VertexArray* VertexBuffer::getTexCoords(int unit,midp::array<float>& scaleBias) const { float* data=scaleBias!=nullptr ? &scaleBias[0] : nullptr; return getTexCoords(unit,data); }

void VertexBuffer::setTexCoords(int unit,VertexArray* array,float scale,const float* bias) {
    if(!verifyTextureUnit(unit) || !verifyMutable()) return;
    VertexArrayTextureUnit*& slot=m_texCoords[unit]; VertexArrayTextureUnit* previous=slot;
    VertexArrayTextureUnit* replacement=array ? new VertexArrayTextureUnit(array,scale,bias) : nullptr;
    slot=replacement; delete previous; updateVertexCount();
}

void VertexBuffer::setTexCoords(int unit,VertexArray* array,float scale,const midp::array<float>& bias) { setTexCoords(unit,array,scale,bias.m_length ? &bias[0] : nullptr); }

void VertexBuffer::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target); VertexBuffer* result=static_cast<VertexBuffer*>(target);
    result->setDefaultColor(getDefaultColor()); float position[4]; VertexArray* array=getPositions(position); result->setPositions(array,position[0],position+1);
    result->setColors(getColors()); result->setNormals(getNormals()); result->setPointSizes(getPointSizes());
    for(int i=0;i<2;++i) { float coords[4]; array=getTexCoords(i,coords); result->setTexCoords(i,array,coords[0],coords+1); }
}

void VertexBuffer::commit() {
    if(!m_mutable) return;
    midp::array<float> position(4); VertexArray* positions=getPositions(position); midp::INCREF(positions);
    VertexArray* normals=getNormals(); midp::INCREF(normals); VertexArray* colors=getColors(); midp::INCREF(colors);
    midp::array<float> coords0(4); VertexArray* texture0=getTexCoords(0,coords0); midp::INCREF(texture0);
    midp::array<float> coords1(4); VertexArray* texture1=getTexCoords(1,coords1); midp::INCREF(texture1);
    int normalOffset=alignOffset(positions->getVertexDataSize());
    int colorOffset=alignOffset(normalOffset+(normals ? normals->getVertexDataSize() : 0));
    int texture0Offset=alignOffset(colorOffset+(colors ? colors->getVertexDataSize() : 0));
    int texture1Offset=alignOffset(texture0Offset+(texture0 ? texture0->getVertexDataSize() : 0));
    int stride=alignOffset(texture1Offset+(texture1 ? texture1->getVertexDataSize() : 0));
    int count=getVertexCount(); void* data=::operator new[](count*stride);
    VertexArray* output=createSharedVertexArray(positions,data,0,stride);
    float scale=position[0]; midp::array<float> bias(3); bias[0]=position[1]; bias[1]=position[2]; bias[2]=position[3]; setPositions(output,scale,bias);
    if(normals) setNormals(createSharedVertexArray(normals,data,normalOffset,stride));
    if(colors) setColors(createSharedVertexArray(colors,data,colorOffset,stride));
    if(texture0) { output=createSharedVertexArray(texture0,data,texture0Offset,stride); scale=coords0[0]; midp::array<float> b(3); b[0]=coords0[1]; b[1]=coords0[2]; b[2]=coords0[3]; setTexCoords(0,output,scale,b); }
    if(texture1) { output=createSharedVertexArray(texture1,data,texture1Offset,stride); scale=coords1[0]; midp::array<float> b(3); b[0]=coords1[1]; b[1]=coords1[2]; b[2]=coords1[3]; setTexCoords(1,output,scale,b); }
    midp::DECREF(positions); midp::DECREF(normals); midp::DECREF(colors); midp::DECREF(texture0); midp::DECREF(texture1);
    m_sharedData=data;
    if(midp::OpenGLESManager::getGLES()->getGLESVersion()==2 && getVertexCount()*stride>256) {
        if(!m_bufferObject) midp::OpenGLESManager::getGLES()->glGenBuffers(1, &m_bufferObject);
        midp::OpenGLESManager::getGLES()->glBindBuffer(0x8892, m_bufferObject);
        midp::GLES11* gl=midp::OpenGLESManager::getGLES(); count=getVertexCount();
        gl->glBufferData(0x8892, count*stride, m_sharedData, 0x88e4);
        midp::OpenGLESManager::getGLES()->glBindBuffer(0x8892, 0);
    }
    m_mutable=false;
}

VertexArray* VertexBuffer::getColors() const { return m_colors; }

VertexArray* VertexBuffer::getNormals() const { return m_normals; }

VertexArray* VertexBuffer::getPointSizes() const { return m_pointSizes; }

VertexArray* VertexBuffer::getSkinIndices() const { return m_skinIndices; }

VertexArray* VertexBuffer::getSkinWeights() const { return m_skinWeights; }

int VertexBuffer::getVertexCount() const { return m_vertexCount; }

int VertexBuffer::getDefaultColor() const { return m_defaultColor; }

int VertexBuffer::getBufferObject() const { return m_bufferObject; }

void VertexBuffer::setPointSizes(VertexArray* array) { midp::INCREF(array); midp::DECREF(m_pointSizes); m_pointSizes=array; updateVertexCount(); }

void VertexBuffer::setNormals(VertexArray* array) { if(!verifyMutable()) return; midp::INCREF(array); midp::DECREF(m_normals); m_normals=array; updateVertexCount(); }

void VertexBuffer::setColors(VertexArray* array) { if(!verifyMutable()) return; midp::INCREF(array); midp::DECREF(m_colors); m_colors=array; updateVertexCount(); }

}
