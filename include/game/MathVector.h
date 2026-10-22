#pragma once

namespace midp {
class DataInputStream;
}

class MathVector {
public:
    MathVector();
    MathVector(float x, float y, float z);
    MathVector(const MathVector& o);
    ~MathVector();

    MathVector& operator=(const MathVector& o);
    MathVector& operator+=(const MathVector& o);
    MathVector& operator-=(const MathVector& o);
    MathVector& operator*=(float s);
    MathVector operator+(const MathVector& o) const;
    MathVector operator-(const MathVector& o) const;
    MathVector operator*(float s) const;
    MathVector operator/(float s) const;

    void set(float x, float y, float z);
    void set(const MathVector& o);
    void set(midp::DataInputStream* in);
    void setF(midp::DataInputStream* in);
    void setAsLinearInterpolation(const MathVector& a, const MathVector& b, float t);
    void setLength(float len);
    void normalise();
    void reverse();
    void rotateYAxis(float angle);

    float dot(const MathVector& o) const;
    MathVector cross(const MathVector& o) const;
    void cross(const MathVector& o, MathVector& out) const;
    float getLength() const;
    float getLengthSq() const;
    bool isZero() const;
    bool isOrthogonal() const;

    float x;
    float y;
    float z;
};
