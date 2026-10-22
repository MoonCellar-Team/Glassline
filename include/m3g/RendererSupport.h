#pragma once
#include "m3g/GLHelpers.h"
#include "m3g/Texture2D.h"
#include "runtime/midp/GLES11.h"
#include "runtime/midp/GLES20.h"
#include "runtime/nio/ByteBuffer.h"
#include "runtime/nio/FloatBuffer.h"
#include "m3g/VertexBuffer.h"
#include "m3g/IndexBuffer.h"
#include "m3g/OptimizedSkinningEngine.h"
#include "m3g/VertexHelpers.h"
#include "m3g/NioBuffers.h"
extern "C" {
void glDrawElements(unsigned,int,unsigned,const void*);
void glDrawArrays(unsigned,int,int);
void glVertexPointer(int,unsigned,int,const void*);
void glColorPointer(int,unsigned,int,const void*);
void glNormalPointer(unsigned,int,const void*);
void glTexCoordPointer(int,unsigned,int,const void*);
void glMatrixIndexPointerOES(int,unsigned,int,const void*);
void glWeightPointerOES(int,unsigned,int,const void*);
void glVertexAttribPointer(unsigned,int,unsigned,unsigned char,int,const void*);
void glLoadMatrixf(const float*); void glLoadIdentity(); void glPushMatrix(); void glPopMatrix();
void glTranslatef(float,float,float); void glScalef(float,float,float);
void glColor4f(float,float,float,float); void glClearColorx(int,int,int,int); void glClear(unsigned);
void glTexEnvf(unsigned,unsigned,float); void glTexEnvx(unsigned,unsigned,int);
void glFogx(unsigned,int); void glFogf(unsigned,float); void glFogfv(unsigned,const float*);
}
namespace m3g {
template<int C> void applySkinWeights(int,float*,float*,const float*,const signed char*,const float*,const Transform* const*);
void uploadTransformPalette(const midp::array<Transform*>*,const float*,int,midp::GLES11*);
inline unsigned vertexType(int type,bool unsignedIntegers=false) {
    if(type==1) return unsignedIntegers ? 0x1401 : 0x1400;
    if(type==2) return unsignedIntegers ? 0x1403 : 0x1402;
    if(type==3) return 0x140c;
    return 0x1406;
}
inline const void* vertexPointer(const VertexArray* array,bool vbo) {
    return vbo ? reinterpret_cast<const void*>(static_cast<__UINTPTR_TYPE__>(array->getDataOffset())) : array->getData();
}
template<class GL> inline void drawIndexBuffer(GL* gl,const IndexBuffer* buffer) {
    const auto& strips=buffer->getStripLengths(); const auto& indices=buffer->getExplicitIndices();
    bool stripped=buffer->isStripped(), implicit=buffer->isImplicit(); int primitive=buffer->getPrimitiveType();
    unsigned mode=primitive==9 ? (stripped ? 3 : 1) : (primitive==8 ? (stripped ? 5 : 4) : 0);
    if(!stripped) {
        int count=buffer->getPrimitiveCount(); if(primitive==9) count*=2; else if(primitive==8) count*=3;
        if(implicit) glDrawArrays(mode,buffer->getFirstIndex(),count);
        else {
            const unsigned short* data=&indices[0];
            if(!buffer->getBufferObject()) glDrawElements(mode,count,0x1403,data);
            else { gl->glBindBuffer(0x8893,buffer->getBufferObject()); glDrawElements(mode,count,0x1403,nullptr); gl->glBindBuffer(0x8893,0); }
        }
    } else {
        int count=strips.m_length; const unsigned short* lengths=&strips[0];
        if(implicit) { int first=buffer->getFirstIndex(); for(int i=0;i<count;++i) { int length=lengths[i]; glDrawArrays(mode,first,length); first+=length; } }
        else { const unsigned short* data=&indices[0]; int offset=0; for(int i=0;i<count;++i) { int previous=offset; offset+=lengths[i]; glDrawElements(mode,lengths[i],0x1403,data+previous); } }
    }
}
inline nio::FloatBuffer* allocateSkinBuffer(int floats) {
    nio::ByteBuffer* bytes=nio::ByteBuffer::allocateDirect(floats*4); bytes->order(nio::ByteOrder::nativeOrder());
    nio::FloatBuffer* buffer=bytes->asFloatBuffer(); midp::INCREF(buffer); return buffer;
}
inline void softwareSkin(nio::FloatBuffer*& buffer,const VertexArray* positions,const VertexArray* indices,const VertexArray* weights,
    const midp::array<Transform*>* palette,const float* scaleBias,bool requireFour) {
    int count=positions->getVertexCount(), components=positions->getComponentCount(), type=positions->getComponentType();
    int floats=components*count;
    if(!buffer || buffer->capacity()<floats) { midp::DECREF(buffer); buffer=allocateSkinBuffer(floats); }
    buffer->limit(floats); buffer->rewind(); int bones=indices->getComponentCount(), weightType=weights->getComponentType();
    const Transform* const* transforms=reinterpret_cast<const Transform* const*>(&(*palette)[0]);
    if(type==4 && weightType==4 && components==3 && (!requireFour || bones==4)) {
        OptimizedSkinningEngine::setMatrixPalette(palette); OptimizedSkinningEngine::setInput(positions,indices,weights);
        OptimizedSkinningEngine::processSkin(buffer,count,bones);
    } else if((type==4 || type==2) && (weightType==4 || weightType==2)) {
        for(int i=0;i<count;++i) {
            signed char packed[4]; float weight[4]; indices->get(i,1,packed);
            if(weightType==4) weights->get(i,1,weight);
            else { unsigned short fixed[4]; weights->get(i,1,reinterpret_cast<short*>(fixed)); for(int j=0;j<bones;++j) weight[j]=fixed[j] ? static_cast<float>(fixed[j])*0.000015259021893143654f : 0.0f; }
            float vertex[4], scaled[4], result[4], scratch[4];
            if(type==4) { positions->get(i,1,vertex); scaleAndBiasVertex<3,float>(scaleBias,scaled,vertex); }
            else { short fixed[4]; positions->get(i,1,fixed); scaleAndBiasVertex<3,short>(scaleBias,scaled,fixed); }
            applySkinWeights<3>(bones,result,scratch,scaled,packed,weight,transforms); buffer->put(result,0,components);
        }
    }
    buffer->rewind();
}
inline void textureFiltering(const Texture2D* texture,int& mag,int& min) {
    int level=texture->getLevelFilter(), image=texture->getImageFilter(); mag=0x2600; min=0x2600;
    if(image==0xd2) min=level==0xd1 ? 0x2702 : (level==0xd2 ? 0x2700 : 0x2600);
    else if(image==0xd1) { mag=0x2601; min=level==0xd1 ? 0x2703 : (level==0xd2 ? 0x2701 : 0x2601); }
}
}
