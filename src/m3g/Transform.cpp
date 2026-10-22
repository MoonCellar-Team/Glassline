#include "m3g/Transform.h"
#include "m3g/TransformClass.h"
#include "m3g/VertexHelpers.h"
#include "runtime/midp/Math.h"

namespace m3g {

Transform::Transform() { setIdentity(); }

Transform::Transform(const Transform* other) { set(other); }

Transform::~Transform() {}

midp::Class* Transform::getClass() const { return new TransformClass; }

void Transform::setIdentity() {
    static const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    set(identity);
}

void Transform::set(const Transform* other) { set(other->m_matrix); }

float Transform::cofactor2(int r1, int r2, int c1, int c2, const float* matrix) {
    return matrix[r1 * 4 + c1] * matrix[r2 * 4 + c2] - matrix[r1 * 4 + c2] * matrix[r2 * 4 + c1];
}

float Transform::determinant3x3() const {
    float first = m_matrix[0] * cofactor2(1,2,1,2,m_matrix);
    float second = first + m_matrix[1] * cofactor2(1,2,2,0,m_matrix);
    return second + m_matrix[2] * cofactor2(1,2,0,1,m_matrix);
}

void Transform::postTranslate(float x, float y, float z) {
    for (int row = 0; row < 3; ++row) {
        float* m = m_matrix + row * 4;
        m[3] = ((m[3] + m[0] * x) + m[1] * y) + m[2] * z;
    }
}

void Transform::postScale(float x, float y, float z) {
    for (int row = 0; row < 4; ++row) { float* m = m_matrix + row * 4; m[0] *= x; m[1] *= y; m[2] *= z; }
}

void Transform::transform4(const float* matrix, float* vector, int offset) {
    float* out = vector + offset;
    float x = out[0], y = out[1], z = out[2], w = out[3];
    for (int row = 0; row < 4; ++row) {
        const float* m = matrix + row * 4;
        out[row] = ((m[0] * x + m[1] * y) + m[2] * z) + m[3] * w;
    }
}

void Transform::transform(float* vectors, int count) const { for (int i = 0; i < count; i += 4) transform4(m_matrix, vectors, i); }

void Transform::postMultiply(const Transform* other) { VFPMatrixMultiply(m_matrix, other->m_matrix, m_matrix); }

void Transform::invert3x3() {
    float scale = 1.0f / determinant3x3();
    float cofactors[9];
    const int pairs[3][2] = {{1,2},{2,0},{0,1}};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            cofactors[row * 3 + col] = cofactor2(pairs[row][0], pairs[row][1], pairs[col][0], pairs[col][1], m_matrix);
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) m_matrix[row * 4 + col] = scale * cofactors[col * 3 + row];
}

void Transform::invert() {
    float x = m_matrix[3], y = m_matrix[7], z = m_matrix[11];
    m_matrix[3] = m_matrix[7] = m_matrix[11] = 0.0f;
    m_matrix[12] = m_matrix[13] = m_matrix[14] = 0.0f; m_matrix[15] = 1.0f;
    invert3x3(); postTranslate(-x, -y, -z);
}

void Transform::postMultiply_34(Transform* other) {
    for (int row = 0; row < 3; ++row) {
        float* m = m_matrix + row * 4;
        float a = fdot3<4>(m, other->m_matrix);
        float b = fdot3<4>(m, other->m_matrix + 1);
        float c = fdot3<4>(m, other->m_matrix + 2);
        float d = fdot3<4>(m, other->m_matrix + 3);
        m[3] = d + m[3]; m[0] = a; m[1] = b; m[2] = c;
    }
}

void Transform::angleAxisToQuat(float angle, float x, float y, float z, float* out) {
    if (angle != 0.0f && x == 0.0f && y == 0.0f && z == 0.0f) { for (int i = 0; i < 4; ++i) out[i] = 0.0f; return; }
    float radians = (midp::Math::PI * angle) * 0.0027777778450399637f;
    float sine = midp::Math::sin(radians); float cosine = midp::Math::cos(radians);
    float invLength = 1.0f / midp::Math::sqrt((x*x + y*y) + z*z);
    out[0] = (sine * x) * invLength; out[1] = (sine * y) * invLength;
    out[2] = (sine * z) * invLength; out[3] = cosine;
}

void Transform::angleAxisToQuatx(int angle, int x, int y, int z, int* out) {
    if (angle == 0 || (x == 0 && y == 0 && z == 0)) { out[0] = out[1] = out[2] = 0; out[3] = 65536; return; }
    float radians = (static_cast<float>(angle) * midp::Math::PI) * 0.00000004238552619906288f;
    float sine = midp::Math::sin(radians); float cosine = midp::Math::cos(radians);
    float fx = static_cast<float>(x), fy = static_cast<float>(y), fz = static_cast<float>(z);
    float sx = fx * 0.0000152587890625f, sy = fy * 0.0000152587890625f, sz = fz * 0.0000152587890625f;
    float invLength = 1.0f / midp::Math::sqrt((sx*sx + sy*sy) + sz*sz);
    out[0] = static_cast<int>((fx * sine) * invLength + 0.5f);
    out[1] = static_cast<int>((fy * sine) * invLength + 0.5f);
    out[2] = static_cast<int>((fz * sine) * invLength + 0.5f);
    out[3] = static_cast<int>(cosine * 65536.0f + 0.5f);
}

void Transform::postRotateQuat(float x, float y, float z, float w) {
    float invLength = 1.0f / midp::Math::sqrt(((x*x + y*y) + z*z) + w*w);
    x = invLength*x; y = invLength*y; z = invLength*z; w = invLength*w;
    float xy = x*y, xz = x*z, yy = y*y, zz = z*z, yz = y*z, yw = y*w, zw = z*w;
    Transform rotation;
    float* m = rotation.m_matrix;
    m[0] = 1.0f - (((yy+yy)+zz)+zz);
    m[1] = (xy+xy) - (zw+zw);
    m[4] = ((xy+xy)+zw)+zw;
    m[2] = ((xz+xz)+yw)+yw;
    float xx2 = x*x + x*x;
    m[5] = 1.0f - ((xx2+zz)+zz);
    float xw2 = x*w + x*w;
    m[6] = (yz+yz) - xw2;
    m[9] = (yz+yz) + xw2;
    m[8] = (xz+xz) - (yw+yw);
    m[10] = 1.0f - ((xx2+yy)+yy);
    postMultiply_34(&rotation);
}

void Transform::postRotateQuatx(int x, int y, int z, int w) {
    postRotateQuat(static_cast<float>(x)*0.0000152587890625f, static_cast<float>(y)*0.0000152587890625f,
        static_cast<float>(z)*0.0000152587890625f, static_cast<float>(w)*0.0000152587890625f);
}

void Transform::postRotate(float angle, float x, float y, float z) { float q[4]; angleAxisToQuat(angle,x,y,z,q); postRotateQuat(q[0],q[1],q[2],q[3]); }

void Transform::set(const float* matrix) {
    { float a = matrix[0], b = matrix[1], c = matrix[2], d = matrix[3], e = matrix[4], f = matrix[5];
      m_matrix[5] = f; m_matrix[4] = e; m_matrix[0] = a; m_matrix[1] = b; m_matrix[2] = c; m_matrix[3] = d; }
    { float a = matrix[6], b = matrix[7], c = matrix[8], d = matrix[9], e = matrix[10], f = matrix[11];
      m_matrix[11] = f; m_matrix[10] = e; m_matrix[6] = a; m_matrix[7] = b; m_matrix[8] = c; m_matrix[9] = d; }
    { float a = matrix[12], b = matrix[13], c = matrix[14], d = matrix[15];
      m_matrix[15] = d; m_matrix[12] = a; m_matrix[13] = b; m_matrix[14] = c; }
}

void Transform::get(float* matrix) const {
    { float a = m_matrix[0], b = m_matrix[1], c = m_matrix[2], d = m_matrix[3], e = m_matrix[4], f = m_matrix[5];
      matrix[5] = f; matrix[4] = e; matrix[0] = a; matrix[1] = b; matrix[2] = c; matrix[3] = d; }
    { float a = m_matrix[6], b = m_matrix[7], c = m_matrix[8], d = m_matrix[9], e = m_matrix[10], f = m_matrix[11];
      matrix[11] = f; matrix[10] = e; matrix[6] = a; matrix[7] = b; matrix[8] = c; matrix[9] = d; }
    { float a = m_matrix[12], b = m_matrix[13], c = m_matrix[14], d = m_matrix[15];
      matrix[15] = d; matrix[12] = a; matrix[13] = b; matrix[14] = c; }
}

}
