#pragma once

#include "runtime/midp/ReferenceCounted.h"

namespace midp {

class ReferenceCountedHardReference {
public:
    explicit ReferenceCountedHardReference(ReferenceCounted* obj);
    ~ReferenceCountedHardReference();

    ReferenceCounted* m_obj;
};

}
