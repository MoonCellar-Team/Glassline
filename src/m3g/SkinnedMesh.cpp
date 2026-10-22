#include "m3g/SkinnedMesh.h"
#include "m3g/SkinnedMeshClass.h"
#include "m3g/VertexBuffer.h"
#include "m3g/GLHelpers.h"
#include "m3g/Object3DFinder.h"
#include "m3g/Placement.h"
#include "runtime/midp/Math.h"

namespace m3g {

SkinnedMesh::SkinnedMesh() : m_skeleton(nullptr),m_legacy(true),m_bones(nullptr),m_bindTransforms(nullptr),m_weights(nullptr),m_totalWeights(),m_transformPool(nullptr),m_boneTransforms(),m_boneArray(),m_skinIndices(nullptr),m_skinWeights(nullptr) {}

SkinnedMesh::~SkinnedMesh() {
    midp::DECREF(m_skeleton); m_skeleton=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_bones)); m_bones=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_bindTransforms)); m_bindTransforms=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_weights)); m_weights=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_transformPool)); m_transformPool=nullptr;
    midp::DECREF(m_skinIndices); m_skinIndices=nullptr; midp::DECREF(m_skinWeights); m_skinWeights=nullptr;
}

midp::Class* SkinnedMesh::getClass() const { return new SkinnedMeshClass; }

int SkinnedMesh::getM3GUniqueClassID() const { return 16; }

Group* SkinnedMesh::getSkeleton() const { return m_skeleton; }

void SkinnedMesh::setLegacy(bool legacy) { m_legacy=legacy; }

bool SkinnedMesh::isLegacy() const { return m_legacy; }

midp::array<int>* SkinnedMesh::getWeights(Node* bone) const { return m_weights->get(bone); }

Transform* SkinnedMesh::getBoneTransform(Node* bone) const { return m_bindTransforms->get(bone); }

void SkinnedMesh::requireVertexBufferNotNull() const { getVertexBuffer(); }

const midp::array<Node*>& SkinnedMesh::getBoneArray() const {
    if(m_boneArray==nullptr || m_boneArray.m_length!=m_bones->size()) m_boneArray=midp::array<Node*>(m_bones->size());
    m_bones->toArray(m_boneArray); return m_boneArray;
}

void SkinnedMesh::addWeight(Node* bone,int weight,int first,int count) {
    requireVertexBufferNotNull(); int vertices=getVertexBuffer()->getVertexCount();
    if(!m_weights) { m_weights=new util::HashtableSE<Node,midp::array<int>>(11); midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_weights)); }
    midp::array<int>* weights=m_weights->get(bone); if(!weights) weights=new midp::array<int>(vertices);
    if(m_totalWeights.m_length==0) m_totalWeights=midp::array<int>(vertices);
    for(int i=first;i<first+count;++i) {
        int old=(*weights)[i]; int& total=m_totalWeights[i]; int previous=total;
        total=previous-midp::Math::abs(old); int updated=old+weight; int& nextTotal=m_totalWeights[i];
        int previousTotal=nextTotal; nextTotal=midp::Math::abs(updated)+previousTotal; (*weights)[i]=updated;
    }
    m_weights->put(bone,weights);
}

void SkinnedMesh::addBone(Node* bone) {
    if(!m_bindTransforms) { m_bindTransforms=new util::HashtableSE<Node,Transform>(11); midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_bindTransforms)); }
    if(!m_bones) { m_bones=new util::VectorSE<Node>; midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_bones)); }
    if(m_bindTransforms->containsKey(bone)) return;
    Transform* transform=new Transform; getTransformTo(bone,transform); m_bindTransforms->put(bone,transform); m_bones->addElement(bone);
}

void SkinnedMesh::addTransformWithoutWeightCheck(Node* bone,int weight,int first,int count) { addBone(bone); addWeight(bone,weight,first,count); }

void SkinnedMesh::addTransform(Node* bone,int weight,int first,int count) { addTransformWithoutWeightCheck(bone,weight,first,count); }

VertexArray* SkinnedMesh::getSkinIndices() const {
    requireVertexBufferNotNull(); VertexBuffer* vertices=getVertexBuffer(); VertexArray* original=vertices->getSkinIndices(); if(original) return original;
    if(!m_skinIndices) {
        int count=vertices->getVertexCount(); m_skinIndices=new VertexArray(count,4,1); midp::INCREF(m_skinIndices);
        midp::array<Node*> bones(getBoneArray());
        for(int vertex=0;vertex<count;++vertex) {
            int weights[4]={0,0,0,0}, indices[4]={0,0,0,0};
            for(int bone=0;bone<bones.m_length;++bone) {
                midp::array<int>* boneWeights=getWeights(bones[bone]); int weight=midp::Math::abs((*boneWeights)[vertex]);
                if(weight==0) continue;
                for(int slot=0;slot<4;++slot) if(weight>weights[slot]) {
                    for(int j=3;j>slot;--j) { weights[j]=weights[j-1]; indices[j]=indices[j-1]; }
                    weights[slot]=weight; indices[slot]=bone; break;
                }
            }
            signed char packed[4]; for(int i=0;i<4;++i) packed[i]=static_cast<signed char>(indices[i]);
            m_skinIndices->set(vertex,1,packed);
        }
    }
    return m_skinIndices;
}

VertexArray* SkinnedMesh::getSkinWeights() const {
    requireVertexBufferNotNull(); VertexBuffer* vertices=getVertexBuffer(); VertexArray* original=vertices->getSkinWeights(); if(original) return original;
    if(!m_skinWeights) {
        VertexArray* indices=getSkinIndices(); int components=indices->getComponentCount(), count=vertices->getVertexCount();
        m_skinWeights=new VertexArray(count,components,4); midp::INCREF(m_skinWeights); midp::array<Node*> bones(getBoneArray());
        for(int vertex=0;vertex<count;++vertex) {
            int weights[4]={0,0,0,0}; signed char packed[4]; indices->get(vertex,1,packed);
            int sum=0, minimum=0;
            for(int slot=0;slot<4;++slot) { int weight=(*getWeights(bones[static_cast<unsigned char>(packed[slot])]))[vertex]; minimum=midp::Math::min(minimum,weight); weights[slot]=weight; sum+=midp::Math::abs(weight); }
            float scale=1.0f; if(sum>0) scale/=static_cast<float>(sum);
            float output[4];
            for(int slot=0;slot<components;++slot) output[slot]=sum<1 ? (slot==0 ? 1.0f : 0.0f) : static_cast<float>(weights[slot]-minimum)*scale;
            m_skinWeights->set(vertex,1,output);
        }
    }
    return m_skinWeights;
}

midp::array<Transform*>* SkinnedMesh::getBoneTransforms() const {
    const midp::array<Node*>& bones=getBoneArray(); int count=bones.m_length;
    if(!m_transformPool) { m_transformPool=new util::VectorSE<Transform>(count); midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_transformPool)); }
    m_transformPool->ensureCapacity(count); while(m_transformPool->size()<count) m_transformPool->addElement(new Transform);
    midp::array<Transform*> transforms(m_boneTransforms);
    if(transforms==nullptr || transforms.m_length!=count) { transforms=midp::array<Transform*>(count); m_boneTransforms=transforms; }
    for(int i=0;i<count;++i) { Node* bone=bones[i]; Transform* transform=m_transformPool->elementAt(i); bone->getTransformTo(this,transform); transform->postMultiply(getBoneTransform(bone)); transforms[i]=transform; }
    return &m_boneTransforms;
}

void SkinnedMesh::setSkeleton(Group* skeleton) {
    requireVertexBufferNotNull(); midp::INCREF(skeleton); Group* previous=m_skeleton; m_skeleton=skeleton; midp::DECREF(previous);
    m_skeleton->setParent(this);
    if(isLegacy()) { Node* bone=m_skeleton; int count=getVertexBuffer()->getVertexCount(); addTransformWithoutWeightCheck(bone,0,0,count); }
}

void SkinnedMesh::animateReferences(int time) const { if(!isRenderingEnabled()) return; Mesh::animateReferences(time); getSkeleton()->animate(time); }

void SkinnedMesh::findReferences(Object3DFinder& finder) const { Mesh::findReferences(finder); finder.find(getSkeleton()); }

int SkinnedMesh::getReferences(midp::array<Object3D*>& references) const { int count=Mesh::getReferences(references); Group* skeleton=getSkeleton(); if(references!=nullptr && skeleton) references[count]=skeleton; return count+(skeleton!=nullptr); }

void SkinnedMesh::duplicateToAddWeight(SkinnedMesh* target,Node* original,Node* copy) {
    midp::array<int>* weights=m_weights->get(original);
    if(weights) { int count=weights->m_length; int* values=&(*weights)[0]; for(int i=0;i<count;++i) if(values[i]) target->addTransform(copy,values[i],i,1); }
    Group* source=m3g_cast<Group>(original); if(!source) return; Group* destination=m3g_cast<Group>(copy);
    int count=source->getChildCount(); for(int i=0;i<count;++i) { Node* left=source->getChild(i); Node* right=destination->getChild(i); duplicateToAddWeight(target,left,right); }
}

void SkinnedMesh::duplicateTo(Object3D* target) {
    Mesh::duplicateTo(target); Group* original=getSkeleton(); Group* copy=static_cast<Group*>(original->duplicate());
    SkinnedMesh* result=static_cast<SkinnedMesh*>(target); result->setSkeleton(copy); duplicateToAddWeight(result,original,copy);
}

}
