#pragma once
#include "runtime/midp/Thread.h"
#include "runtime/midp/ReferenceCounted.h"
namespace midp {
class ReferenceCountedSoftReference {
public:
    explicit ReferenceCountedSoftReference(const ReferenceCounted* object);
    ~ReferenceCountedSoftReference();
    const ReferenceCounted* m_object;
};
}
