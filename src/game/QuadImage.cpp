#include "game/QuadImage.h"

#include <stddef.h>

#include "m3g/Image2D.h"

QuadImage::QuadImage()
    : m_image(NULL), m_resource(-1), m_refCount(0), m_texScale(0), m_size(-1), m_scaleU(-1), m_scaleV(-1) {}

QuadImage::QuadImage(const QuadImage& other)
    : m_image(other.m_image),
      m_resource(other.m_resource),
      m_refCount(other.m_refCount),
      m_texScale(other.m_texScale),
      m_size(other.m_size),
      m_scaleU(other.m_scaleU),
      m_scaleV(other.m_scaleV) {}

QuadImage::~QuadImage() {
    midp::DECREF(m_image);
}
