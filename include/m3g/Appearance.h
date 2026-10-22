#pragma once
#include "m3g/AppearanceBase.h"
namespace m3g { class CustomShader; class PointSpriteMode; class Texture2D; class Material; class Fog; }

namespace m3g {

class Appearance : public AppearanceBase {
public:
    Appearance();
    virtual ~Appearance();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    void setFog(Fog* fog);
    void setMaterial(Material* material);
    void setPointSpriteMode(PointSpriteMode* mode);
    void setCustomShader(CustomShader* shader);
    CustomShader* getCustomShader();
    Fog* getFog() const;
    Material* getMaterial() const;
    PointSpriteMode* getPointSpriteMode() const;
    void setTexture(int index,Texture2D* texture);
    Texture2D* getTexture(int index) const;
    virtual void animateReferences(int time) const;
    virtual void findReferences(Object3DFinder& finder) const;
    virtual int getReferences(midp::array<Object3D*>& references) const;
    virtual void duplicateTo(Object3D* target);
    CustomShader* m_customShader;
    PointSpriteMode* m_pointSpriteMode;
    midp::array<Texture2D*> m_textures;
    Material* m_material;
    Fog* m_fog;
};

}
