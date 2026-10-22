#include "m3g/Node.h"
#include "m3g/GroupClass.h"
#include "m3g/Object3DFinder.h"
#include "m3g/Placement.h"
extern "C" int __cxa_atexit(void (*)(void*),void*,void*);
extern "C" void* __dso_handle;
namespace m3g {
alignas(GroupClass) static unsigned char s_classStorage[sizeof(GroupClass)];

Group::Group() : m_children(nullptr), m_childCount(0) {}

Group::~Group() {
    int count=getChildCount();
    while(count>0) { --count; removeChild(getChild(count)); }
    util::VectorSE<midp::Object>* children=m_children; m_children=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(children));
}

midp::Class* Group::getClass() const { return reinterpret_cast<GroupClass*>(s_classStorage); }

int Group::getChildCount() const { return m_childCount; }

Node* Group::getChild(int index) const { return static_cast<Node*>(m_children->elementAt(index)); }

int Group::getM3GUniqueClassID() const { return 9; }

void Group::addChild(Node* child) {
    if(!m_children) {
        m_children=new util::VectorSE<midp::Object>;
        midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(m_children));
    }
    child->setParent(this); m_children->addElement(child); m_childCount=m_children->size();
}

void Group::removeChild(Node* child) {
    child->setParent(nullptr);
    if(!m_children) return;
    m_children->removeElement(child); m_childCount=m_children->size();
}

void Group::animateReferences(int time) const { if(isRenderingEnabled()) animateReferencesGroup(time); }

void Group::animateReferencesGroup(int time) const {
    if(getChildCount()==0) return;
    midp::Object** current=m_children->begin(); midp::Object** end=m_children->end();
    do { static_cast<Object3D*>(*current)->animate(time); ++current; } while(current!=end);
}

void Group::findReferences(Object3DFinder& finder) const {
    Object3D::findReferences(finder);
    for(int i=getChildCount();i>0;) { --i; finder.find(getChild(i)); }
}

int Group::getReferences(midp::array<Object3D*>& references) const {
    int base=Object3D::getReferences(references); int count=getChildCount();
    if(references!=nullptr) for(int i=0;i<getChildCount();++i) references[base+i]=getChild(i);
    return count+base;
}

void Group::duplicateTo(Object3D* target) {
    Node::duplicateTo(target);
    for(int i=0;i<getChildCount();++i) {
        Node* child=static_cast<Node*>(getChild(i)->duplicate());
        static_cast<Group*>(target)->addChild(child);
    }
}

static void cleanupGroupClass(void*) { reinterpret_cast<GroupClass*>(s_classStorage)->~GroupClass(); }

static void initializeGroupClass(int, int) {
    new (s_classStorage) GroupClass;
    __cxa_atexit(cleanupGroupClass,nullptr,&__dso_handle);
}

__attribute__((constructor)) static void initializeGroupClassWrapper() { initializeGroupClass(0,0); }

}
