#pragma once
#include "m3g/VertexArray.h"
#include "m3g/Transform.h"
namespace nio { class FloatBuffer; }
namespace m3g {
class OptimizedSkinningEngine {
public:
    static void setInput(const VertexArray* positions,const VertexArray* indices,const VertexArray* weights);
    static void processSkin(nio::FloatBuffer* output,unsigned vertices,unsigned unused);
    static void setMatrixPalette(const midp::array<Transform*>* transforms);
    static void uploadTransposed(const Transform* transform,float* output);
    static const VertexArray* s_sourcePositions;
    static const VertexArray* s_sourceIndices;
    static const VertexArray* s_sourceWeights;
};
}
