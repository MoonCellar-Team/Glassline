#pragma once
#include "m3g/Object3D.h"
namespace midp { class Image; }
namespace m3g { class Image2DPlatformData; }

namespace m3g {

class Image2D : public Object3D {
public:
    Image2D();
    Image2D(int format,midp::Image* image);
    virtual ~Image2D();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    virtual void duplicateTo(Object3D* target);
    int getNumMipMapLevels() const;
    void* getMipMapData(int level) const;
    int getMipMapWidth(int level) const;
    int getMipMapHeight(int level) const;
    int getMipMapDataSize(int level) const;
    int getFormat() const;
    int getWidth() const;
    int getHeight() const;
    bool isMutable() const;
    void commit(const midp::array<signed char>& palette,const signed char* indices);
    void commit(const unsigned char* pixels);
    void uploadImage();
    void discard();
    void set(int format,int width,int height);
    void set(int format,const midp::Image* image);
    Image2DPlatformData* m_platform;
    int m_format;
    int m_width;
    int m_height;
    bool m_mutable;
    int m_bitsPerPixel;
    int m_levels;
    unsigned char** m_data;
    int m_dataLevels;
};

}
