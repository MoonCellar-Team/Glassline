#pragma once
#include "m3g/Node.h"
#include "m3g/OpaqueObject.h"
#include "runtime/util/VectorSE.h"
namespace m3g { class VertexBuffer; class IndexBuffer; class Appearance; class AppearanceBase; }

namespace m3g {

using MeshIndexBuffers = OpaqueObject<util::VectorSE<midp::Object>, 24>;
using MeshAppearances = OpaqueObject<util::VectorSE<midp::Object>, 24>;

class Mesh : public Node {
public:
    Mesh();
    Mesh(int submeshes,int unused);
    Mesh(VertexBuffer* vertices,IndexBuffer* indices,Appearance* appearance);
    virtual ~Mesh();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    bool verifyMutable() const;
    bool isMutable() const;
    bool verifyIndex(int index) const;
    VertexBuffer* getVertexBuffer() const;
    int getSubmeshCount() const;
    IndexBuffer* getIndexBuffer(int index) const;
    Appearance* getAppearance(int index) const;
    void setVertexBuffer(VertexBuffer* vertices);
    void setIndexBuffer(int index,IndexBuffer* indices);
    void setAppearanceBase(int index,AppearanceBase* appearance);
    void setAppearance(int index,Appearance* appearance);
    void setSubmeshCount(int count);
    virtual void preparePositionsForSkinning();
    virtual void commit();
    virtual void animateReferences(int time) const;
    virtual void findReferences(Object3DFinder& finder) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateTo(Object3D* target);
    util::VectorSE<midp::Object>& indices() const { return *const_cast<util::VectorSE<midp::Object>*>(m_indexBuffers.get()); }
    util::VectorSE<midp::Object>& appearances() const { return *const_cast<util::VectorSE<midp::Object>*>(m_appearances.get()); }
    VertexBuffer* m_vertices;
    MeshIndexBuffers m_indexBuffers;
    MeshAppearances m_appearances;
    bool m_mutable;
};

}
