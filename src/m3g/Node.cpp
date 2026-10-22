#include "m3g/Node.h"
#include "runtime/midp/ReferenceCountedSoftReference.h"
#include "m3g/Placement.h"
namespace m3g {
static util::VectorSE<Node>* s_nodeList=nullptr;
static volatile bool s_nodeListLocked=false;
#ifdef MEDECOMP_PORT
static void lockNodeList() { while(__atomic_exchange_n(&s_nodeListLocked,true,__ATOMIC_ACQUIRE)) midp::Thread::yield(); }
static void unlockNodeList() { __atomic_store_n(&s_nodeListLocked,false,__ATOMIC_RELEASE); }
#else
static void lockNodeList() { while(s_nodeListLocked) midp::Thread::yield(); s_nodeListLocked=true; }
static void unlockNodeList() { s_nodeListLocked=false; }
#endif
bool getPathToParent(const Node*,const Node*,util::VectorSE<Node>*);
}

namespace m3g {

Node::Node() : m_parent(nullptr), m_rendering(true), m_alphaFactor(65536), m_scope(-1) {
    if (!s_nodeList) {
        s_nodeList=new util::VectorSE<Node>;
        midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(s_nodeList));
    }
}

Node::~Node() { m_parent=nullptr; }

void Node::setPickingEnable(bool) {}

float Node::getAlphaFactor() const { return static_cast<float>(getAlphaFactorx())*0.0000152587890625f; }

void Node::setAlphaFactor(float alpha) { setAlphaFactorx(static_cast<int>(alpha*65536.0f+0.5f)); }

void Node::updateAnimationProperty(int property, const midp::array<int>& value) {
    Transformable::updateAnimationProperty(property,value);
    if(property==0x114) setRenderingEnable(value[0]>0x7fff);
    else if(property==0x100) setAlphaFactorx(value[0]);
}

void Node::updateAnimationProperty(int property, const midp::array<float>& value) {
    Transformable::updateAnimationProperty(property,value);
    if(property==0x114) setRenderingEnable(value[0]>0.5f);
    else if(property==0x100) setAlphaFactor(value[0]);
}

void Node::duplicateTo(Object3D* target) { Transformable::duplicateTo(target); static_cast<Node*>(target)->setAlphaFactorx(getAlphaFactorx()); }

bool Node::getTransformTo(const Node* target, Transform* out) const {
    midp::ReferenceCountedSoftReference sourceRef(this), targetRef(target);
    lockNodeList();
    util::VectorSE<Node>* list=s_nodeList;
    bool forward=getPathToParent(this,target,list);
    if(!forward) {
        list->removeAllElements();
        if(!getPathToParent(target,this,list)) { list->removeAllElements(); unlockNodeList(); return false; }
    }
    out->setIdentity();
    for(int i=list->size();i>0;) { --i; list->elementAt(i)->getCompositeTransformCumulative(out); }
    if(!forward) out->invert();
    list->removeAllElements(); unlockNodeList(); return true;
}

int Node::getScope() const { return m_scope; }

bool Node::isRenderingEnabled() const { return m_rendering; }

Node* Node::getParent() const { return m_parent; }

int Node::getAlphaFactorx() const { return m_alphaFactor; }

void Node::setScope(int value) { m_scope=value; }

void Node::setRenderingEnable(bool value) { m_rendering=value; }

void Node::setParent(Node* value) { m_parent=value; }

void Node::setAlphaFactorx(int value) { m_alphaFactor=value; }

}
