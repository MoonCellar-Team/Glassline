#pragma once

#include "runtime/midp/ReferenceCounted.h"

class CollShape;
class MathVector;

namespace midp {
class DataInputStream;
}

class FallBox : public midp::ReferenceCounted {
public:
    explicit FallBox(midp::DataInputStream& in);
    virtual ~FallBox();

    bool contains(const MathVector& point);

    CollShape* m_shape;
};
