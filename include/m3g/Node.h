#pragma once
#include "m3g/Transformable.h"

namespace m3g {

class Node : public Transformable {
public:
    Node();
    virtual ~Node();
    void setRenderingEnable(bool enable);
    void setPickingEnable(bool enable);
    bool isRenderingEnabled() const;
    Node* getParent() const;
    void setParent(Node* parent);
    bool getTransformTo(const Node* target, Transform* out) const;
    void setAlphaFactor(float alpha);
    void setAlphaFactorx(int alpha);
    float getAlphaFactor() const;
    int getAlphaFactorx() const;
    void setScope(int scope);
    int getScope() const;
    virtual void updateAnimationProperty(int property, const midp::array<float>& value);
    virtual void updateAnimationProperty(int property, const midp::array<int>& value);
    virtual void duplicateTo(Object3D* target);
    Node* m_parent;
    bool m_rendering;
    int m_alphaFactor;
    int m_scope;
};

template<class T> T* const m3g_cast(Object3D*);
class Group : public Node {
public:
    Group();
    virtual ~Group();
    virtual midp::Class* getClass() const;
    void addChild(Node* child);
    void removeChild(Node* child);
    Node* getChild(int index) const;
    int getChildCount() const;
    virtual void animateReferences(int time) const;
    void animateReferencesGroup(int time) const;
    virtual void findReferences(Object3DFinder& finder) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateTo(Object3D* target);
    virtual int getM3GUniqueClassID() const;
    util::VectorSE<midp::Object>* m_children;
    int m_childCount;
};
}
