#include "game/MathVector.h"

#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Math.h"

MathVector::MathVector() : x(0), y(0), z(0) {}

MathVector::MathVector(float x, float y, float z) : x(x), y(y), z(z) {}

MathVector::MathVector(const MathVector& o) : x(o.x), y(o.y), z(o.z) {}

MathVector::~MathVector() {}

MathVector& MathVector::operator=(const MathVector& o) {
    x = o.x;
    y = o.y;
    z = o.z;
    return *this;
}

MathVector& MathVector::operator+=(const MathVector& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}

MathVector& MathVector::operator-=(const MathVector& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}

MathVector& MathVector::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

MathVector MathVector::operator+(const MathVector& o) const {
    MathVector r;
    r.x = x + o.x;
    r.y = y + o.y;
    r.z = z + o.z;
    return r;
}

MathVector MathVector::operator-(const MathVector& o) const {
    MathVector r;
    r.x = x - o.x;
    r.y = y - o.y;
    r.z = z - o.z;
    return r;
}

MathVector MathVector::operator*(float s) const {
    MathVector r;
    r.x = x * s;
    r.y = y * s;
    r.z = z * s;
    return r;
}

MathVector MathVector::operator/(float s) const {
    MathVector r;
    r.x = x / s;
    r.y = y / s;
    r.z = z / s;
    return r;
}

void MathVector::set(float nx, float ny, float nz) {
    x = nx;
    y = ny;
    z = nz;
}

void MathVector::set(const MathVector& o) {
    x = o.x;
    y = o.y;
    z = o.z;
}

void MathVector::set(midp::DataInputStream* in) {
    x = in->readFloat();
    y = in->readFloat();
    z = in->readFloat();
}

void MathVector::setF(midp::DataInputStream* in) {
    const float scale = 1.0f / 65536.0f;
    x = in->readInt() * scale;
    y = in->readInt() * scale;
    z = in->readInt() * scale;
}

void MathVector::setAsLinearInterpolation(const MathVector& a, const MathVector& b, float t) {
    x = a.x * (1.0f - t) + b.x * t;
    y = a.y * (1.0f - t) + b.y * t;
    z = a.z * (1.0f - t) + b.z * t;
}

float MathVector::dot(const MathVector& o) const {
    return x * o.x + y * o.y + z * o.z;
}

void MathVector::cross(const MathVector& o, MathVector& out) const {
    out.set(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
}

MathVector MathVector::cross(const MathVector& o) const {
    return MathVector(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
}

float MathVector::getLengthSq() const {
    return x * x + y * y + z * z;
}

float MathVector::getLength() const {
    return midp::Math::sqrt(x * x + y * y + z * z);
}

void MathVector::normalise() {
    float length = midp::Math::sqrt(x * x + y * y + z * z);
#ifdef MEDECOMP_PORT
    if (length == 0.0f)
        return;
#endif
    float inv = 1.0f / length;
    x *= inv;
    y *= inv;
    z *= inv;
}

void MathVector::setLength(float len) {
    float length = midp::Math::sqrt(x * x + y * y + z * z);
#ifdef MEDECOMP_PORT
    if (length == 0.0f)
        return;
#endif
    float k = len / length;
    x *= k;
    y *= k;
    z *= k;
}

void MathVector::reverse() {
    x = -x;
    y = -y;
    z = -z;
}

void MathVector::rotateYAxis(float angle) {
    float s = midp::Math::sin(angle);
    float c = midp::Math::cos(angle);
    float nx = c * x - s * z;
    float nz = c * z + s * x;
    x = nx;
    z = nz;
}

bool MathVector::isZero() const {
    return x == 0 && y == 0 && z == 0;
}

bool MathVector::isOrthogonal() const {
    if (x != 0 && y != 0 && z != 0) {
        return false;
    }
    return ((x != 0) ^ (y != 0)) != (z != 0);
}
