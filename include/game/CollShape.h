#pragma once

#include "game/MathOrthoBox.h"
#include "runtime/midp/ReferenceCounted.h"

class MathLine;
class MathVector;
class SeperatedAxesList;
class SeperatedAxis;

namespace midp {
template <class T>
class array;
}

class CollShape : public midp::ReferenceCounted {
public:
    enum ShapeType {
        SHAPE_ORTHO_HEXAHEDRON = 0,
        SHAPE_RAMP_NEG_X = 1,
        SHAPE_RAMP_POS_X = 2,
        SHAPE_OCTAHEDRON = 3,
    };

    CollShape(ShapeType type, float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    virtual ~CollShape();
    virtual bool intersects(const MathLine& line, float* tMin, float* tMax) const = 0;
    virtual void addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const = 0;
    virtual void setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const = 0;

    float getBounce();
    float getInverseResistance();
    const MathOrthoBox& getBounds() const;
    void setPosition(const MathVector& position);
    const MathOrthoBox& getLocalBounds() const;
    ShapeType getShapeType();
    bool isRamp();
    void setBounce(float bounce);
    void setRangeOnAxis(SeperatedAxis* axis, int index) const;
    bool intersects(const MathVector& point) const;
    bool pointIntersects(const MathVector& point) const;

    static SeperatedAxesList* getSepAxisList(int index);
    static bool moveShape(CollShape* shape, MathVector& position, const MathVector& move,
                          midp::array<CollShape*>& shapes, int from, int to, float& multiple, CollShape** hit,
                          MathVector& normal);
    static bool testIntersection(CollShape* a, CollShape* b);

    static SeperatedAxesList* s_sepAxisList[3];

    MathOrthoBox m_localBounds;
    MathOrthoBox m_bounds;
    float m_bounce;
    float m_inverseResistance;
    int m_material;
    ShapeType m_shapeType;
};

class CollOrthoHexahedron : public CollShape {
public:
    CollOrthoHexahedron(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    virtual ~CollOrthoHexahedron();
    virtual bool intersects(const MathLine& line, float* tMin, float* tMax) const;
    virtual void addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const;
    virtual void setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const;
};

class CollOctahedron : public CollShape {
public:
    CollOctahedron(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    virtual ~CollOctahedron();
    virtual bool intersects(const MathLine& line, float* tMin, float* tMax) const;
    virtual void addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const;
    virtual void setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const;
};

class CollOrthoRampPosX : public CollShape {
public:
    CollOrthoRampPosX(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    virtual ~CollOrthoRampPosX();
    virtual bool intersects(const MathLine& line, float* tMin, float* tMax) const;
    virtual void addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const;
    virtual void setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const;
};

class CollOrthoRampNegX : public CollShape {
public:
    CollOrthoRampNegX(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    virtual ~CollOrthoRampNegX();
    virtual bool intersects(const MathLine& line, float* tMin, float* tMax) const;
    virtual void addNonOrthogonalAxesTo(SeperatedAxesList* list, int index) const;
    virtual void setNonOrthogonalRangeOnAxis(SeperatedAxis* axis, int index) const;
};
