#pragma once
#include "m3g/Object3D.h"
#include "m3g/VertexArray.h"
namespace m3g { class VertexArrayTextureUnit; }

namespace m3g {

class VertexBuffer : public Object3D {
public:
    VertexBuffer();
    virtual ~VertexBuffer();
    virtual midp::Class* getClass() const;
    virtual int getM3GUniqueClassID() const;
    void commit();
    bool verifyMutable() const;
    int getDefaultColor() const;
    int getVertexCount() const;
    int getBufferObject() const;
    void setDefaultColor(int color);
    VertexArray* getPositions(float* scaleBias) const;
    VertexArray* getPositions(midp::array<float>& scaleBias) const;
    VertexArray* getColors() const;
    VertexArray* getNormals() const;
    VertexArray* getPointSizes() const;
    VertexArray* getSkinIndices() const;
    VertexArray* getSkinWeights() const;
    void updateVertexCount();
    void setPositions(VertexArray* array,float scale,const float* bias);
    void setPositions(VertexArray* array,float scale,midp::array<float> bias);
    void setPointSizes(VertexArray* array);
    void setNormals(VertexArray* array);
    void setColors(VertexArray* array);
    VertexArray* getTexCoords(int unit,float* scaleBias) const;
    VertexArray* getTexCoords(int unit,midp::array<float>& scaleBias) const;
    void setTexCoords(int unit,VertexArray* array,float scale,const float* bias);
    void setTexCoords(int unit,VertexArray* array,float scale,const midp::array<float>& bias);
    virtual void duplicateTo(Object3D* target);
    int m_vertexCount;
    int m_defaultColor;
    VertexArray* m_positions;
    float m_positionScale;
    float m_positionBias[3];
    VertexArray* m_colors;
    VertexArray* m_normals;
    VertexArray* m_pointSizes;
    midp::array<VertexArrayTextureUnit*> m_texCoords;
    VertexArray* m_skinIndices;
    VertexArray* m_skinWeights;
    int m_bufferObject;
    bool m_mutable;
    void* m_sharedData;
};

}
