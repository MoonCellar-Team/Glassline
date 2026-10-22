#include "m3g/Image2DPlatformData.h"

namespace m3g {

Image2DPlatformData::Image2DPlatformData() : texture(0), magFilter(0), minFilter(0), wrapS(0), wrapT(0), dirty(false), m_reserved{}, destructor(nullptr) { texture = 0; }

Image2DPlatformData::~Image2DPlatformData() { if (destructor) destructor(this); }

}
