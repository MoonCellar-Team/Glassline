#include "m3g/Mesh.h"
#include "m3g/LoaderHelpers.h"
#include "m3g/Appearance.h"
#include "m3g/IndexBuffer.h"
#include "m3g/Object3DFinder.h"
#include "m3g/MeshClass.h"
#include "m3g/Placement.h"

namespace m3g {

Mesh::Mesh() : m_vertices(nullptr),m_mutable(true) {}

Mesh::Mesh(int submeshes,int) : m_vertices(nullptr),m_mutable(true) { setSubmeshCount(submeshes); }

Mesh::Mesh(VertexBuffer* vertices,IndexBuffer* indexBuffer,Appearance* appearance) : m_vertices(nullptr),m_mutable(true) { setVertexBuffer(vertices); setSubmeshCount(1); setIndexBuffer(0,indexBuffer); setAppearance(0,appearance); }

Mesh::~Mesh() {
    midp::DECREF(m_vertices); m_vertices=nullptr;
    indices().removeAllElements(); appearances().removeAllElements();
}

midp::Class* Mesh::getClass() const { return new MeshClass; }

int Mesh::getM3GUniqueClassID() const { return 14; }

bool Mesh::verifyMutable() const { return m_mutable!=false; }

bool Mesh::isMutable() const { return m_mutable; }

bool Mesh::verifyIndex(int index) const { return index>=0 && index<getSubmeshCount(); }

VertexBuffer* Mesh::getVertexBuffer() const { return m_vertices; }

int Mesh::getSubmeshCount() const { return indices().size(); }

IndexBuffer* Mesh::getIndexBuffer(int index) const { return verifyIndex(index) ? static_cast<IndexBuffer*>(indices().elementAt(index)) : nullptr; }

Appearance* Mesh::getAppearance(int index) const { return verifyIndex(index) ? static_cast<Appearance*>(appearances().elementAt(index)) : nullptr; }

void Mesh::setVertexBuffer(VertexBuffer* vertices) { if(!verifyMutable()) return; midp::INCREF(vertices); midp::DECREF(m_vertices); m_vertices=vertices; }

void Mesh::setIndexBuffer(int index,IndexBuffer* buffer) { if(verifyMutable() && verifyIndex(index)) indices().setElementAt(buffer,index); }

void Mesh::setAppearanceBase(int index,AppearanceBase* appearance) { if(verifyMutable() && verifyIndex(index)) appearances().setElementAt(appearance,index); }

void Mesh::setAppearance(int index,Appearance* appearance) { setAppearanceBase(index,appearance); }

void Mesh::setSubmeshCount(int count) {
    if(!verifyMutable()) return;
    indices().removeAllElements(); indices().setSize(count);
    appearances().removeAllElements(); appearances().setSize(count);
}

void Mesh::animateReferences(int time) const { if(isRenderingEnabled()) getVertexBuffer()->animate(time); }

void Mesh::findReferences(Object3DFinder& finder) const {
    Object3D::findReferences(finder); finder.find(getVertexBuffer());
    int count=getSubmeshCount();
    for(int i=0;i<count;++i) finder.find(getAppearance(i));
    for(int i=0;i<count;++i) finder.find(getIndexBuffer(i));
}

int Mesh::getReferences(midp::array<Object3D*>& references) const {
    int count=Object3D::getReferences(references); int total=count;
    if(getSubmeshCount()>0) {
        int submeshes=getSubmeshCount();
        for(int i=0;i<submeshes;++i) { bool appearance=getAppearance(i)!=nullptr; bool index=getIndexBuffer(i)!=nullptr; total+=appearance+index; }
    }
    if(references!=nullptr) {
        for(int i=0;i<getSubmeshCount();++i) {
            Appearance* appearance=getAppearance(i); if(appearance) references[count++]=appearance;
            IndexBuffer* index=getIndexBuffer(i); if(index) references[count++]=index;
        }
        references[count]=getVertexBuffer();
    }
    return total+1;
}

void Mesh::duplicateTo(Object3D* target) {
    Node::duplicateTo(target); Mesh* result=static_cast<Mesh*>(target);
    result->setSubmeshCount(getSubmeshCount());
    VertexBuffer* vertices=getVertexBuffer(); if(vertices) result->setVertexBuffer(vertices);
    for(int i=0;i<getSubmeshCount();++i) { result->setAppearance(i,getAppearance(i)); result->setIndexBuffer(i,getIndexBuffer(i)); }
    result->m_mutable=isMutable();
}

void Mesh::commit() {
    if(!m_mutable) return;
    VertexBuffer* vertices=static_cast<VertexBuffer*>(getVertexBuffer()->duplicate()); vertices->commit(); setVertexBuffer(vertices);
    for(int i=0;i<getSubmeshCount();++i) { IndexBuffer* buffer=getIndexBuffer(i)->commitDuplicate(); setIndexBuffer(i,buffer); }
    m_mutable=false;
}

void Mesh::preparePositionsForSkinning() {
    float scaleBias[4]; VertexArray* source=getVertexBuffer()->getPositions(scaleBias);
    if(source->getComponentCount()!=3) return;
    if(source->getComponentType()==4 && scaleBias[0]==1.0f && scaleBias[1]==0.0f && scaleBias[2]==0.0f && scaleBias[3]==0.0f) return;
    int count=source->getVertexCount(); int components=source->getComponentCount(); midp::array<float> values(count*components);
    count=source->getVertexCount(); components=source->getComponentCount(); VertexArray* output=new VertexArray(count,components,4);
    source->get(0,source->getVertexCount(),values);
    count=source->getVertexCount();
    if(count>0) {
        float* value=static_cast<float*>(values.data());
        for(int i=0;i<source->getVertexCount();++i,value+=3) {
            value[0]=value[0]*scaleBias[0]+scaleBias[1]; value[1]=value[1]*scaleBias[0]+scaleBias[2]; value[2]=value[2]*scaleBias[0]+scaleBias[3];
        }
        count=source->getVertexCount();
    }
    output->set(0,count,values); float bias[3]={0,0,0}; getVertexBuffer()->setPositions(output,1.0f,bias);
}

}
