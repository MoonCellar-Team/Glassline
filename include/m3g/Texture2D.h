#pragma once
#include "m3g/Transformable.h"
namespace m3g { class Image2D; }

namespace m3g {

class Texture2D : public Transformable {
public:
    Texture2D();
    explicit Texture2D(Image2D* image);
    virtual ~Texture2D();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    virtual void duplicateTo(Object3D* target);
    int getBlending() const;
    Image2D* getImage() const;
    int getImageFilter() const;
    int getLevelFilter() const;
    int getWrappingS() const;
    int getWrappingT() const;
    void setImage(Image2D* image);
    void setBlendColor(int color);
    void setBlending(int blending);
    void setFiltering(int levelFilter,int imageFilter);
    void setWrapping(int s,int t);
    Image2D* m_image;
    int m_blendColor;
    int m_blending;
    int m_imageFilter;
    int m_levelFilter;
    int m_wrappingS;
    int m_wrappingT;
};

}
