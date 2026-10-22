#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class VertexArray : public Object3D {
public:
    VertexArray();
    VertexArray(int vertices,int components,int type);
    VertexArray(int vertices,int components,int type,void* data,int offset,int stride);
    virtual ~VertexArray();
    virtual int getM3GUniqueClassID() const;
    int getComponentCount() const;
    int getComponentType() const;
    int getVertexCount() const;
    void* getData() const;
    int getVertexDataSize() const;
    int getDataStride() const;
    int getComponentStride() const;
    int getDataOffset() const;
    void setFormat(int vertices,int components,int type);
    static void* allocate(int vertices,int components,int type);
    static void deallocate(void* data,int type);
    void set(int first,int count,const signed char* values);
    void set(int first,int count,midp::array<signed char> values);
    void get(int first,int count,midp::array<signed char>& values) const;
    void get(int first,int count,signed char* values) const;
    void set(int first,int count,const short* values);
    void set(int first,int count,midp::array<short> values);
    void get(int first,int count,midp::array<short>& values) const;
    void get(int first,int count,short* values) const;
    void set(int first,int count,const int* values);
    void set(int first,int count,midp::array<int> values);
    void get(int first,int count,midp::array<int>& values) const;
    void set(int first,int count,const float* values);
    void set(int first,int count,midp::array<float> values);
    void get(int first,int count,midp::array<float>& values) const;
    void get(int first,int count,float* values) const;
    int m_vertexCount;
    int m_componentCount;
    int m_componentType;
    void* m_data;
    bool m_shared;
    int m_offset;
    int m_stride;
    int m_componentStride;
};

}
