#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class IndexBuffer : public Object3D {
public:
    IndexBuffer();
    IndexBuffer(int type,int primitives,const midp::array<int>& indices);
    IndexBuffer(int type,int primitives,int first);
    IndexBuffer(int type,const midp::array<int>& lengths,int first);
    virtual ~IndexBuffer();
    void commit();
    virtual IndexBuffer* commitDuplicate() const;
    void setFirstIndex(int first);
    void setPrimitiveCount(int primitives);
    void setPrimitiveType(int type);
    void getIndices(midp::array<int>& indices) const;
    midp::array<int> collapseStripsImplicit(const midp::array<int>& lengths,int first);
    midp::array<int> collapseStrips(const midp::array<int>& lengths,const midp::array<int>& indices);
    void setIndices(const midp::array<int>& indices);
    void setStripLengths(const midp::array<int>& lengths);
    virtual int getM3GUniqueClassID() const;
    int getBufferObject() const;
    int getIndexCount() const;
    int getPrimitiveCount() const;
    int getPrimitiveType() const;
    int getFirstIndex() const;
    const midp::array<unsigned short>& getExplicitIndices() const;
    const midp::array<unsigned short>& getStripLengths() const;
    bool isStripped() const;
    bool isImplicit() const;
    int m_firstIndex;
    int m_primitiveCount;
    midp::array<unsigned short> m_indices;
    midp::array<unsigned short> m_stripLengths;
    bool m_committed;
    int m_primitiveType;
    int m_bufferObject;
};

}
