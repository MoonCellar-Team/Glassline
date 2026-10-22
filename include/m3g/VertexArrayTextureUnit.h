#pragma once
namespace m3g { class VertexArray; }

namespace m3g {

class VertexArrayTextureUnit {
public:
    VertexArrayTextureUnit(VertexArray* array,float scale,const float* bias);
    virtual ~VertexArrayTextureUnit();
    int getVertexCount();
    VertexArray* m_array;
    float m_scale;
    float m_bias[2];
};

}
