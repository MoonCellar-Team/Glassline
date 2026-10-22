#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/array.h"

namespace midp {
class DataInputStream;
}
class PathPoint;

class Path : public midp::ReferenceCounted {
public:
    explicit Path(midp::DataInputStream& in);
    virtual ~Path();

    const PathPoint* getPoint(int index) const;
    int getPointCount() const;

    midp::array<PathPoint*> m_points;
};
