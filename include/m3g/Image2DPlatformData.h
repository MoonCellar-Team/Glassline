#pragma once

namespace m3g {

class Image2DPlatformData {
public:
    Image2DPlatformData();
    ~Image2DPlatformData();
    int texture;
    int magFilter;
    int minFilter;
    int wrapS;
    int wrapT;
    bool dirty;
    unsigned char m_reserved[3];
    void (*destructor)(Image2DPlatformData*);
};

}
