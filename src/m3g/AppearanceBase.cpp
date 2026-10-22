#include "m3g/AppearanceBase.h"
#include "m3g/CompositingMode.h"
#include "m3g/PolygonMode.h"

namespace m3g {

AppearanceBase::AppearanceBase() : m_layer(0), m_lineMode(nullptr),m_polygonMode(nullptr),m_compositingMode(nullptr) {}

AppearanceBase::~AppearanceBase() { midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_lineMode)); midp::DECREF(m_polygonMode); midp::DECREF(m_compositingMode); }

void AppearanceBase::setLayer(int layer) { m_layer=layer; }

int AppearanceBase::getLayer() const { return m_layer; }

void AppearanceBase::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target); AppearanceBase* result=static_cast<AppearanceBase*>(target);
    result->setLayer(getLayer()); result->setCompositingMode(getCompositingMode());
    result->setPolygonMode(getPolygonMode()); result->setLineMode(getLineMode());
}

int AppearanceBase::getReferences(midp::array<Object3D*>& references) const {
    int count=Object3D::getReferences(references);
    bool line=m_lineMode!=nullptr, polygon=m_polygonMode!=nullptr, compositing=m_compositingMode!=nullptr;
    int total=count+line+polygon+compositing;
    if(references!=nullptr) {
        if(m_compositingMode) references[count++]=m_compositingMode;
        if(m_polygonMode) references[count++]=m_polygonMode;
        if(m_lineMode) references[count]=reinterpret_cast<Object3D*>(m_lineMode);
    }
    return total;
}

LineMode* AppearanceBase::getLineMode() const { return m_lineMode; }

void AppearanceBase::setLineMode(LineMode* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_lineMode)); m_lineMode=value;
}

PolygonMode* AppearanceBase::getPolygonMode() const { return m_polygonMode; }

void AppearanceBase::setPolygonMode(PolygonMode* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_polygonMode)); m_polygonMode=value;
}

CompositingMode* AppearanceBase::getCompositingMode() const { return m_compositingMode; }

void AppearanceBase::setCompositingMode(CompositingMode* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_compositingMode)); m_compositingMode=value;
}

}
