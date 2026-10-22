#include "m3g/VertexArrayTextureUnit.h"
#include "m3g/GLHelpers.h"

namespace m3g {

VertexArrayTextureUnit::VertexArrayTextureUnit(VertexArray* array,float scale,const float* bias) : m_array(array),m_scale(1.0f),m_bias{0,0} {
    midp::INCREF(m_array); m_scale=scale;
    m_bias[0]=bias ? bias[0] : 0.0f; m_bias[1]=bias ? bias[1] : 0.0f;
}

VertexArrayTextureUnit::~VertexArrayTextureUnit() { midp::DECREF(m_array); }

int VertexArrayTextureUnit::getVertexCount() { return m_array->getVertexCount(); }

}
