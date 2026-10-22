#pragma once
#include "m3g/Mesh.h"
#include "m3g/RendererTable.h"
#include "m3g/VertexArray.h"

namespace m3g {

class SkinnedMesh : public Mesh {
public:
    SkinnedMesh();
    virtual ~SkinnedMesh();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    Group* getSkeleton() const;
    void setLegacy(bool legacy);
    bool isLegacy() const;
    midp::array<int>* getWeights(Node* bone) const;
    Transform* getBoneTransform(Node* bone) const;
    void requireVertexBufferNotNull() const;
    const midp::array<Node*>& getBoneArray() const;
    void addWeight(Node* bone,int weight,int first,int count);
    void addBone(Node* bone);
    void addTransformWithoutWeightCheck(Node* bone,int weight,int first,int count);
    void addTransform(Node* bone,int weight,int first,int count);
    VertexArray* getSkinIndices() const;
    VertexArray* getSkinWeights() const;
    midp::array<Transform*>* getBoneTransforms() const;
    void setSkeleton(Group* skeleton);
    virtual void animateReferences(int time) const;
    virtual void findReferences(Object3DFinder& finder) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateToAddWeight(SkinnedMesh* target,Node* original,Node* copy);
    virtual void duplicateTo(Object3D* target);
    Group* m_skeleton;
    bool m_legacy;
    util::VectorSE<Node>* m_bones;
    util::HashtableSE<Node,Transform>* m_bindTransforms;
    util::HashtableSE<Node,midp::array<int>>* m_weights;
    midp::array<int> m_totalWeights;
    mutable util::VectorSE<Transform>* m_transformPool;
    mutable midp::array<Transform*> m_boneTransforms;
    mutable midp::array<Node*> m_boneArray;
    mutable VertexArray* m_skinIndices;
    mutable VertexArray* m_skinWeights;
};

}
