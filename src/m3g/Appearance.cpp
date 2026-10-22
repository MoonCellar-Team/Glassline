#include "m3g/Appearance.h"
#include "m3g/Texture2D.h"
#include "m3g/Fog.h"
#include "m3g/Object3DFinder.h"
#include "m3g/AppearanceClass.h"

namespace m3g {

Appearance::Appearance() : m_customShader(nullptr),m_pointSpriteMode(nullptr),m_textures(2),m_material(nullptr),m_fog(nullptr) { m_textures[0]=nullptr; m_textures[1]=nullptr; }

Appearance::~Appearance() {
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_pointSpriteMode)); m_pointSpriteMode=nullptr;
    for(int i=0;i<2;++i) { Texture2D* texture=m_textures[i]; m_textures[i]=nullptr; midp::DECREF(texture); }
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_material)); m_material=nullptr;
    midp::DECREF(m_fog); m_fog=nullptr;
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_customShader)); m_customShader=nullptr;
}

midp::Class* Appearance::getClass() const { return new AppearanceClass; }

int Appearance::getM3GUniqueClassID() const { return 3; }

void Appearance::setCustomShader(CustomShader* shader) {
    m_customShader=shader; midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(shader));
}

CustomShader* Appearance::getCustomShader() { return m_customShader; }

void Appearance::setTexture(int index,Texture2D* texture) { midp::INCREF(texture); midp::DECREF(m_textures[index]); m_textures[index]=texture; }

Texture2D* Appearance::getTexture(int index) const { return m_textures[index]; }

void Appearance::animateReferences(int time) const {
    Object3D* material=reinterpret_cast<Object3D*>(getMaterial()); if(material) material->animate(time);
    Fog* fog=getFog(); if(fog) fog->animate(time);
    for(int i=0;i<2;++i) { Texture2D* texture=getTexture(i); if(texture) texture->animate(time); }
}

void Appearance::findReferences(Object3DFinder& finder) const {
    Object3D::findReferences(finder);
    finder.find(reinterpret_cast<Object3D*>(getPointSpriteMode()));
    finder.find(reinterpret_cast<Object3D*>(getMaterial())); finder.find(getFog());
    for(int i=0;i<2;++i) finder.find(getTexture(i));
}

int Appearance::getReferences(midp::array<Object3D*>& references) const {
    int count=AppearanceBase::getReferences(references);
    int total=count+(m_pointSpriteMode!=nullptr)+(m_material!=nullptr)+(m_fog!=nullptr);
    for(int i=0;i<2;++i) total+=getTexture(i)!=nullptr;
    if(references!=nullptr) {
        if(m_pointSpriteMode) references[count++]=reinterpret_cast<Object3D*>(m_pointSpriteMode);
        if(m_material) references[count++]=reinterpret_cast<Object3D*>(m_material);
        if(m_fog) references[count++]=m_fog;
        for(int i=0;i<2;++i) { Texture2D* texture=getTexture(i); if(texture) references[count++]=texture; }
    }
    return total;
}

void Appearance::duplicateTo(Object3D* target) {
    AppearanceBase::duplicateTo(target); Appearance* result=static_cast<Appearance*>(target);
    result->setPointSpriteMode(getPointSpriteMode());
    for(int i=0;i<2;++i) result->setTexture(i,getTexture(i));
    result->setMaterial(getMaterial()); result->setFog(getFog()); result->setCustomShader(getCustomShader());
}

Fog* Appearance::getFog() const { return m_fog; }

void Appearance::setFog(Fog* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_fog)); m_fog=value;
}

Material* Appearance::getMaterial() const { return m_material; }

void Appearance::setMaterial(Material* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_material)); m_material=value;
}

PointSpriteMode* Appearance::getPointSpriteMode() const { return m_pointSpriteMode; }

void Appearance::setPointSpriteMode(PointSpriteMode* value) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(value));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_pointSpriteMode)); m_pointSpriteMode=value;
}

}
