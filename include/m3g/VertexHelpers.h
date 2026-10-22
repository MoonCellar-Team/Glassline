#pragma once
#include "m3g/fixedToFloat.h"
#include "m3g/halfToFloat.h"

namespace m3g {
template<class D, class S> void __getVerts(D*, const S*, int, int, int, int);
template<class D, class S> void __getVerts(D*, int, const S*, int, int, int, int);
template<class D, class S> void __setVerts(D*, int, const S*, int, int, int, int);
template<class D, class S, class F> void __getVertsTransform(D*, int, S*, int, int, int, int, F);
template<int Stride> const float fdot3(const float*, const float*);
template<int Components, class S> void scaleAndBiasVertex(const float*, float*, const S*);

void VFPMatrixMultiply(const float*, const float*, float*);
int componentSize(int);
bool verifyTextureUnit(int);
void ensureNoGLError();
void transpose(float*);
void vfpTransformBeta(const float*, float*, float, const float*);
void vfpTransformBetaAccumulate(const float*, float*, float, const float*);
bool isZeroWeight(int);

template<> const float fdot3<4>(const float* a, const float* b);
template<> void __getVerts<float, float>(float* out, const float* source,
    int first, int count, int components, int stride);
template<> void __getVerts<float, signed char>(float* out, const signed char* source,
    int first, int count, int components, int stride);
template<> void __getVerts<float, short>(float* out, const short* source,
    int first, int count, int components, int stride);
template<> void __getVerts<signed char, signed char>(signed char* out, const signed char* source,
    int first, int count, int components, int stride);
template<> void __getVerts<int, int>(int* out, const int* source,
    int first, int count, int components, int stride);
template<> void __getVerts<short, short>(short* out, const short* source,
    int first, int count, int components, int stride);
template<> void __getVerts<signed char, signed char>(signed char* out, int, const signed char* source,
    int first, int count, int components, int stride);
template<> void __getVerts<int, int>(int* out, int, const int* source,
    int first, int count, int components, int stride);
template<> void __getVerts<short, short>(short* out, int, const short* source,
    int first, int count, int components, int stride);
template<> void __setVerts<signed char, signed char>(signed char* destination, int, const signed char* source,
    int first, int count, int components, int stride);
template<> void __setVerts<float, float>(float* destination, int, const float* source,
    int first, int count, int components, int stride);
template<> void __setVerts<int, int>(int* destination, int, const int* source,
    int first, int count, int components, int stride);
template<> void __setVerts<short, short>(short* destination, int, const short* source,
    int first, int count, int components, int stride);
template<> void __getVertsTransform<float, const int, fixedToFloat>(float* out, int,
    const int* source, int first, int count, int components, int stride, fixedToFloat convert);
template<> void __getVertsTransform<float, const short, halfToFloat>(float* out, int,
    const short* source, int first, int count, int components, int stride, halfToFloat convert);
template<> void scaleAndBiasVertex<3, short>(const float* scaleBias, float* out, const short* source);
template<> void scaleAndBiasVertex<3, float>(const float* scaleBias, float* out, const float* source);
}
