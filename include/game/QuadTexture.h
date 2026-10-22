#pragma once

namespace m3g {
class Texture2D;
class VertexArray;
}

class QuadTexture {
public:
    QuadTexture();
    QuadTexture(const QuadTexture& other);
    ~QuadTexture();

    m3g::Texture2D* m_texture;
    m3g::VertexArray* m_texCoords;
    int m_image;
    int m_refCount;
    int m_blending;
    int m_imageFilter;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
};
