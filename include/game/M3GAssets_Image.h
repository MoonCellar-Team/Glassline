#pragma once

namespace m3g {
class Image2D;
}

class M3GAssets_Image {
public:
    explicit M3GAssets_Image(int resId);
    ~M3GAssets_Image();

    m3g::Image2D* loadImage();
    void freeImage();

    int m_resId;
    m3g::Image2D* m_image;
    int m_useCount;
};
