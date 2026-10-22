#pragma once
#include "runtime/midp/Object.h"
namespace midp { class Class; }

namespace m3g {

class Transform : public midp::Object {
public:
    Transform();
    explicit Transform(const Transform* other);
    virtual ~Transform();
    virtual midp::Class* getClass() const;
    void set(const float* matrix);
    void set(const Transform* other);
    void get(float* matrix) const;
    void setIdentity();
    void transform(float* vectors, int count) const;
    static void transform4(const float* matrix, float* vector, int offset);
    static float cofactor2(int r1, int r2, int c1, int c2, const float* matrix);
    float determinant3x3() const;
    void postTranslate(float x, float y, float z);
    void postScale(float x, float y, float z);
    void postMultiply(const Transform* other);
    void postMultiply_34(Transform* other);
    void invert3x3();
    void invert();
    static void angleAxisToQuat(float angle, float x, float y, float z, float* out);
    static void angleAxisToQuatx(int angle, int x, int y, int z, int* out);
    void postRotateQuat(float x, float y, float z, float w);
    void postRotateQuatx(int x, int y, int z, int w);
    void postRotate(float angle, float x, float y, float z);
    typedef float Matrix[16] __attribute__((aligned(16)));
    Matrix m_matrix;
};

}
