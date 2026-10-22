#pragma once

#include "runtime/midp/ReferenceCounted.h"

class CollShape;
class GameObject;

namespace midp {
class DataInputStream;
}

class PainBox : public midp::ReferenceCounted {
public:
    explicit PainBox(midp::DataInputStream& in);
    virtual ~PainBox();

    bool intersects(const GameObject& obj);

    CollShape* m_shape;
};
