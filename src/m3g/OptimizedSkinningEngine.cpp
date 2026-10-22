#include "m3g/OptimizedSkinningEngine.h"
#include "m3g/VertexHelpers.h"
#include "runtime/nio/FloatBuffer.h"
#include "m3g/Placement.h"
extern "C" int __cxa_atexit(void (*)(void*),void*,void*);
extern "C" void* __dso_handle;
namespace m3g {
const VertexArray* OptimizedSkinningEngine::s_sourcePositions=nullptr;
const VertexArray* OptimizedSkinningEngine::s_sourceIndices=nullptr;
const VertexArray* OptimizedSkinningEngine::s_sourceWeights=nullptr;
alignas(midp::array<float>) static unsigned char paletteStorage[sizeof(midp::array<float>)];
static midp::array<float>& palette() { return *reinterpret_cast<midp::array<float>*>(paletteStorage); }

void OptimizedSkinningEngine::setInput(const VertexArray* positions,const VertexArray* indices,const VertexArray* weights) {
    s_sourcePositions=positions; s_sourceIndices=indices; s_sourceWeights=weights;
}

void OptimizedSkinningEngine::processSkin(nio::FloatBuffer* output,unsigned vertices,unsigned) {
    float* destination=output->lockBuffer();
    const float* positions=static_cast<const float*>(s_sourcePositions->getData());
    const float* weights=static_cast<const float*>(s_sourceWeights->getData());
    const signed char* indices=static_cast<const signed char*>(s_sourceIndices->getData());
    int positionStride=s_sourcePositions->getDataStride(), weightStride=s_sourceWeights->getDataStride(), indexStride=s_sourceIndices->getDataStride();
    if(static_cast<int>(vertices)>0) do {
        float input[4]={positions[0],positions[1],positions[2],1.0f};
        positions=reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(positions)+positionStride);
        float transformed[4];
        float* matrices=static_cast<float*>(palette().data());
        vfpTransformBeta(input,transformed,weights[0],matrices+indices[0]*16);
        for(int i=1;i<4;++i) {
            int bits; __builtin_memcpy(&bits,weights+i,sizeof(bits)); if(isZeroWeight(bits)) break;
            matrices=static_cast<float*>(palette().data()); vfpTransformBetaAccumulate(input,transformed,weights[i],matrices+indices[i]*16);
        }
        indices+=indexStride; weights=reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(weights)+weightStride);
        destination[0]=transformed[0]; destination[1]=transformed[1]; destination[2]=transformed[2]; destination+=3;
    } while(--vertices!=0);
    output->unlock();
}

void OptimizedSkinningEngine::setMatrixPalette(const midp::array<Transform*>* transforms) {
    int count=transforms->m_length;
    if(palette().m_length<count*16) { palette()=midp::array<float>(count*16); count=transforms->m_length; }
    for(int i=0;i<count;++i) { Transform* transform=(*transforms)[i]; float* out=static_cast<float*>(palette().data())+i*16; uploadTransposed(transform,out); }
}

void OptimizedSkinningEngine::uploadTransposed(const Transform* transform,float* output) {
    transform->get(output); transpose(output);
}

static void cleanupPalette(void*) { palette().~array(); }

static void initializePalette(int,int) {
    new (paletteStorage) midp::array<float>(0x180);
    __cxa_atexit(cleanupPalette,nullptr,&__dso_handle);
}

__attribute__((constructor)) static void initializePaletteWrapper() { initializePalette(0,0); }
}
