#include "game/QuadTexture.h"

#include <string.h>

#include "m3g/Texture2D.h"
#include "m3g/VertexArray.h"

QuadTexture::QuadTexture() {
    memset(static_cast<void*>(this), 0, sizeof(QuadTexture));
}

QuadTexture::QuadTexture(const QuadTexture& other)
    : m_texture(other.m_texture),
      m_texCoords(other.m_texCoords),
      m_image(other.m_image),
      m_refCount(other.m_refCount),
      m_blending(other.m_blending),
      m_imageFilter(other.m_imageFilter),
      m_x(other.m_x),
      m_y(other.m_y),
      m_width(other.m_width),
      m_height(other.m_height) {}

QuadTexture::~QuadTexture() {
    midp::DECREF(m_texture);
    midp::DECREF(m_texCoords);
}
