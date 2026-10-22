#include "m3g/VertexArray.h"
#include "m3g/VertexHelpers.h"
void* operator new[](__SIZE_TYPE__);
void operator delete[](void*) noexcept;

namespace m3g {

VertexArray::VertexArray() : m_vertexCount(0),m_componentCount(0),m_componentType(0),m_data(nullptr),m_shared(false),m_offset(0),m_stride(0),m_componentStride(0) {}

VertexArray::VertexArray(int vertices,int components,int type) : m_vertexCount(0),m_componentCount(0),m_componentType(0),m_data(nullptr),m_shared(false),m_offset(0),m_stride(0),m_componentStride(0) { setFormat(vertices,components,type); }

VertexArray::VertexArray(int vertices,int components,int type,void* data,int offset,int stride) :
    m_vertexCount(0),m_componentCount(0),m_componentType(0),m_data(data),m_shared(true),m_offset(offset),m_stride(stride),m_componentStride(0) { setFormat(vertices,components,type); }

VertexArray::~VertexArray() { if(!m_shared) deallocate(m_data,getComponentType()); }

int VertexArray::getM3GUniqueClassID() const { return 20; }

void* VertexArray::getData() const { return reinterpret_cast<void*>(reinterpret_cast<__UINTPTR_TYPE__>(m_data)+m_offset); }

int VertexArray::getVertexDataSize() const { int components=getComponentCount(); return componentSize(getComponentType())*components; }

void VertexArray::deallocate(void* data,int type) {
    if(type>=1 && type<=5 && data) ::operator delete[](data);
}

void* VertexArray::allocate(int vertices,int components,int type) {
    int bytes=componentSize(type);
    if(bytes==0) return nullptr;
    return ::operator new[](static_cast<unsigned>(vertices)*static_cast<unsigned>(components)*bytes);
}

void VertexArray::setFormat(int vertices,int components,int type) {
    if(!m_shared && m_data) deallocate(m_data,getComponentType());
    m_vertexCount=vertices; m_componentCount=components; m_componentType=type;
    if(!m_shared) {
        int v=getVertexCount(), c=getComponentCount(), t=getComponentType();
        m_data=allocate(v,c,t);
        int size=componentSize(getComponentType()); m_stride=getComponentCount()*size;
    }
    int stride=getDataStride(); int size=componentSize(getComponentType());
    m_componentStride=stride/size;
}

void VertexArray::get(int first,int count,float* values) const {
    void* data=getData(); int stride=getComponentStride(); int components=getComponentCount();
    switch(getComponentType()) {
    case 1: __getVerts<float,signed char>(values,static_cast<signed char*>(data),first,count,components,stride); break;
    case 2: __getVerts<float,short>(values,static_cast<short*>(data),first,count,components,stride); break;
    case 3: __getVertsTransform<float,const int,fixedToFloat>(values,0,static_cast<const int*>(data),first,count,components,stride,fixedToFloat()); break;
    case 4: __getVerts<float,float>(values,static_cast<float*>(data),first,count,components,stride); break;
    case 5: __getVertsTransform<float,const short,halfToFloat>(values,0,static_cast<const short*>(data),first,count,components,stride,halfToFloat()); break;
    }
}

int VertexArray::getComponentCount() const { return m_componentCount; }

int VertexArray::getComponentType() const { return m_componentType; }

int VertexArray::getVertexCount() const { return m_vertexCount; }

int VertexArray::getDataStride() const { return m_stride; }

int VertexArray::getComponentStride() const { return m_componentStride; }

int VertexArray::getDataOffset() const { return m_offset; }

void VertexArray::set(int first,int count,const signed char* values) {
    signed char* data=static_cast<signed char*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __setVerts<signed char,signed char>(data,0,values,first,count,components,stride);
}

void VertexArray::set(int first,int count,midp::array<signed char> values) { set(first,count,&values[0]); }

void VertexArray::get(int first,int count,midp::array<signed char>& values) const { get(first,count,&values[0]); }

void VertexArray::get(int first,int count,signed char* values) const {
    const signed char* data=static_cast<const signed char*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __getVerts<signed char,signed char>(values,0,data,first,count,components,stride);
}

void VertexArray::set(int first,int count,const short* values) {
    short* data=static_cast<short*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __setVerts<short,short>(data,0,values,first,count,components,stride);
}

void VertexArray::set(int first,int count,midp::array<short> values) { set(first,count,&values[0]); }

void VertexArray::get(int first,int count,midp::array<short>& values) const { get(first,count,&values[0]); }

void VertexArray::get(int first,int count,short* values) const {
    const short* data=static_cast<const short*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __getVerts<short,short>(values,0,data,first,count,components,stride);
}

void VertexArray::set(int first,int count,const int* values) {
    int* data=static_cast<int*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __setVerts<int,int>(data,0,values,first,count,components,stride);
}

void VertexArray::set(int first,int count,midp::array<int> values) { set(first,count,&values[0]); }

void VertexArray::get(int first,int count,midp::array<int>& values) const {
    int* out=&values[0]; const int* data=static_cast<const int*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __getVerts<int,int>(out,0,data,first,count,components,stride);
}

void VertexArray::set(int first,int count,const float* values) {
    float* data=static_cast<float*>(getData()); int components=getComponentCount(); int stride=getComponentStride();
    __setVerts<float,float>(data,0,values,first,count,components,stride);
}

void VertexArray::set(int first,int count,midp::array<float> values) { set(first,count,&values[0]); }

void VertexArray::get(int first,int count,midp::array<float>& values) const { get(first,count,&values[0]); }

}
