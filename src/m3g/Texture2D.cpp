#include "m3g/Texture2D.h"
#include "m3g/GLHelpers.h"
#include "m3g/Texture2DClass.h"

namespace m3g {

Texture2D::Texture2D() : m_image(nullptr),m_blendColor(-1),m_blending(0xe3),m_imageFilter(0xd2),m_levelFilter(0xd0),m_wrappingS(0xf1),m_wrappingT(0xf1) {}

Texture2D::Texture2D(Image2D* image) : m_image(nullptr),m_blendColor(-1),m_blending(0xe3),m_imageFilter(0xd2),m_levelFilter(0xd0),m_wrappingS(0xf1),m_wrappingT(0xf1) { setImage(image); }

Texture2D::~Texture2D() { Image2D* image=m_image; m_image=nullptr; midp::DECREF(image); }

midp::Class* Texture2D::getClass() const { return new Texture2DClass; }

int Texture2D::getM3GUniqueClassID() const { return 17; }

void Texture2D::setImage(Image2D* image) { midp::INCREF(image); midp::DECREF(m_image); m_image=image; }

void Texture2D::setBlendColor(int color) { m_blendColor=color; }

void Texture2D::setBlending(int blending) { m_blending=blending; }

void Texture2D::setFiltering(int levelFilter,int imageFilter) { m_imageFilter=imageFilter; m_levelFilter=levelFilter; }

void Texture2D::setWrapping(int s,int t) { m_wrappingS=s; m_wrappingT=t; }

void Texture2D::duplicateTo(Object3D* target) {
    Transformable::duplicateTo(target); Texture2D* result=static_cast<Texture2D*>(target);
    result->setImage(getImage()); result->setBlending(getBlending());
    int level=getLevelFilter(); int image=getImageFilter(); result->setFiltering(level,image);
    int s=getWrappingS(); int t=getWrappingT(); result->setWrapping(s,t);
}

int Texture2D::getBlending() const { return m_blending; }

Image2D* Texture2D::getImage() const { return m_image; }

int Texture2D::getImageFilter() const { return m_imageFilter; }

int Texture2D::getLevelFilter() const { return m_levelFilter; }

int Texture2D::getWrappingS() const { return m_wrappingS; }

int Texture2D::getWrappingT() const { return m_wrappingT; }

}
