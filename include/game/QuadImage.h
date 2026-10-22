#pragma once

namespace m3g {
class Image2D;
}

class QuadImage {
public:
    QuadImage();
    QuadImage(const QuadImage& other);
    ~QuadImage();

    m3g::Image2D* m_image;
    int m_resource;
    int m_refCount;
    float m_texScale;
    int m_size;
    int m_scaleU;
    int m_scaleV;
};
