#pragma once

#include "game/SeperatedAxis.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/util/VectorSE.h"

class CollShape;
class MathVector;

class SeperatedAxesList : public midp::ReferenceCounted {
public:
    SeperatedAxesList();
    virtual ~SeperatedAxesList();

    static bool checkCombineAxes(SeperatedAxesList* list, CollShape* shape0, CollShape* shape2,
                                 CollShape* shape1, SeperatedAxis** touching, bool firstThreeOnly,
                                 const MathVector& move);
    static void storeStaticBounds(SeperatedAxesList* from, SeperatedAxesList* to);
    int addRangeToAxesWithCheck(CollShape* shape, int index);
    void combineTValues();
    bool noIntersection(int a, int b);
    static void getMoveOutMultiple(SeperatedAxesList* list1, SeperatedAxesList* list2, const MathVector& move,
                                   int index, float& multiple, SeperatedAxis** axis);
    SeperatedAxis* newAxis();
    void addAxes(SeperatedAxesList* other);
    void addAxis(const MathVector& direction, int index, float minT, float maxT);
    void addOrthogonalAxes(CollShape* shape);
    void reset();
    int getAxisNum();
    SeperatedAxis* getAxis(int index);

    union { util::VectorSE<SeperatedAxis> m_axes; char m_axesBytes[0x18]; };
    int m_count;
};
