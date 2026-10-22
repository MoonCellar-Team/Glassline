#include "m3g/IndexBuffer.h"
#include "m3g/GLHelpers.h"
#include "runtime/midp/GLES11.h"

namespace m3g {

IndexBuffer::IndexBuffer() : m_firstIndex(-1),m_primitiveCount(-1),m_indices(),m_stripLengths(),m_committed(false),m_primitiveType(0),m_bufferObject(0) {}

IndexBuffer::IndexBuffer(int type,int primitives,const midp::array<int>& indices) : m_firstIndex(-1),m_primitiveCount(-1),m_indices(),m_stripLengths(),m_committed(false),m_primitiveType(0),m_bufferObject(0) { setPrimitiveType(type); setPrimitiveCount(primitives); setIndices(indices); }

IndexBuffer::IndexBuffer(int type,int primitives,int first) : m_firstIndex(-1),m_primitiveCount(-1),m_indices(),m_stripLengths(),m_committed(false),m_primitiveType(0),m_bufferObject(0) { setPrimitiveType(type); setPrimitiveCount(primitives); setFirstIndex(first); }

IndexBuffer::IndexBuffer(int,const midp::array<int>& lengths,int first) :
    m_firstIndex(-1),m_primitiveCount(0),m_indices(),m_stripLengths(),m_committed(false),m_primitiveType(0),m_bufferObject(0) {
    midp::array<int> indices=collapseStripsImplicit(lengths,first); setPrimitiveType(8); setIndices(indices);
}

IndexBuffer::~IndexBuffer() { if(m_bufferObject) { midp::OpenGLESManager::removeBuffer(m_bufferObject); m_bufferObject=0; } }

void IndexBuffer::commit() { m_committed=true; }

IndexBuffer* IndexBuffer::commitDuplicate() const { return const_cast<IndexBuffer*>(this); }

int IndexBuffer::getM3GUniqueClassID() const { return 27; }

void IndexBuffer::getIndices(midp::array<int>& indices) const {
    int count=m_indices.m_length;
    for(int i=0;i<count;++i) { int& destination=indices[i]; destination=m_indices[i]; }
}

void IndexBuffer::setIndices(const midp::array<int>& indices) {
    int count=indices.m_length; m_indices=midp::array<unsigned short>(count);
    for(int i=0;i<count;++i) { unsigned short& destination=m_indices[i]; destination=static_cast<unsigned short>(indices[i]); }
    midp::GLES11* gl=midp::OpenGLESManager::getGLES();
    if(gl->getGLESVersion()==2 && m_indices.m_length>64) {
        if(m_bufferObject==0) midp::OpenGLESManager::getGLES()->glGenBuffers(1, &m_bufferObject);
        midp::OpenGLESManager::getGLES()->glBindBuffer(0x8893, m_bufferObject);
        gl=midp::OpenGLESManager::getGLES(); int bytes=m_indices.m_length*2;
        gl->glBufferData(0x8893, bytes, m_indices.data(), 0x88e4);
        midp::OpenGLESManager::getGLES()->glBindBuffer(0x8893, 0);
    }
}

void IndexBuffer::setStripLengths(const midp::array<int>& lengths) {
    if(m_firstIndex==-1 && m_indices.m_length>0) {
        midp::array<int> indices(m_indices.m_length); getIndices(indices);
        midp::array<int> collapsed=collapseStrips(lengths,indices); setIndices(collapsed);
    } else { midp::array<int> collapsed=collapseStripsImplicit(lengths,m_firstIndex); setIndices(collapsed); }
}

bool IndexBuffer::isStripped() const { return m_stripLengths.m_length>0; }

bool IndexBuffer::isImplicit() const { return m_firstIndex>=0; }

void IndexBuffer::setFirstIndex(int value) { m_firstIndex=value; }

void IndexBuffer::setPrimitiveCount(int value) { m_primitiveCount=value; }

void IndexBuffer::setPrimitiveType(int value) { m_primitiveType=value; }

int IndexBuffer::getBufferObject() const { return m_bufferObject; }

int IndexBuffer::getIndexCount() const { return m_indices.m_length; }

int IndexBuffer::getPrimitiveCount() const { return m_primitiveCount; }

int IndexBuffer::getPrimitiveType() const { return m_primitiveType; }

const midp::array<unsigned short>& IndexBuffer::getExplicitIndices() const { return m_indices; }

const midp::array<unsigned short>& IndexBuffer::getStripLengths() const { return m_stripLengths; }

midp::array<int> IndexBuffer::collapseStripsImplicit(const midp::array<int>& lengths,int first) {
    int primitives=0;
    int strips=lengths.m_length;
    for(int i=0;i<strips;++i) primitives+=lengths[i]-2;
    m_primitiveCount=primitives; midp::array<int> result(primitives*3);
    int cursor=0, base=first;
    for(int strip=0;strip<lengths.m_length;++strip) {
        int length=lengths[strip];
        for(int triangle=0;triangle<length-2;++triangle) {
            int a=base+triangle, b=base+triangle+1, c=base+triangle+2;
            int& out=result[cursor];
            if((triangle&1)==0) { out=a; result[cursor+1]=b; }
            else { out=b; result[cursor+1]=a; }
            result[cursor+2]=c;
            cursor+=3; length=lengths[strip];
        }
        base+=length;
    }
    return result;
}

midp::array<int> IndexBuffer::collapseStrips(const midp::array<int>& lengths,const midp::array<int>& indices) {
    int primitives=0;
    int strips=lengths.m_length;
    for(int i=0;i<strips;++i) primitives+=lengths[i]-2;
    m_primitiveCount=primitives; midp::array<int> result(primitives*3);
    int cursor=0, base=0;
    for(int strip=0;strip<lengths.m_length;++strip) {
        int length=lengths[strip];
        for(int triangle=0;triangle<length-2;++triangle) {
            int a=base+triangle, b=base+triangle+1, c=base+triangle+2;
            int& out=result[cursor];
            if((triangle&1)==0) { out=indices[a]; result[cursor+1]=indices[b]; }
            else { out=indices[b]; result[cursor+1]=indices[a]; }
            result[cursor+2]=indices[c];
            cursor+=3; length=lengths[strip];
        }
        base+=length;
    }
    return result;
}

int IndexBuffer::getFirstIndex() const { return m_firstIndex; }

}
