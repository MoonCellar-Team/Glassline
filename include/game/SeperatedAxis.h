#pragma once

#include "game/MathVector.h"
#include "runtime/midp/ReferenceCounted.h"

class MathOrthoBox;

class SeperatedAxis : public midp::ReferenceCounted {
public:
    enum AxisType {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2,
        AXIS_OTHER = 3,
    };

    struct Range {
        float min;
        float max;
    };

    SeperatedAxis();
    virtual ~SeperatedAxis();

    void ensureNoIntersection(MathVector& position, const MathOrthoBox& localBounds, const MathVector& move);
    float getMoveOutMultiple(const MathVector& move, int index);
    void getNormal(const MathVector& move, MathVector& normal);
    void setAxis(SeperatedAxis* other);
    AxisType getType();
    MathVector& getDirection();
    float getMinT(int index);
    float getMaxT(int index);
    Range* getTValues(int index);
    void setTValues(int index, float minT, float maxT);
    bool noIntersection();
    bool noIntersection(int a, int b);
    float getDistanceOfPoint(const MathVector& point);
    void setAxis(const MathVector& direction, int index, float minT, float maxT, AxisType type);
    void setAxis(const MathVector& direction, int index, float minT, float maxT);

    AxisType m_type;
    MathVector m_direction;
    Range m_ranges[3];
};
