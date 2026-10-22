#include "runtime/midp/ReferenceCountedHardReference.h"

namespace midp {

ReferenceCountedHardReference::ReferenceCountedHardReference(ReferenceCounted* obj) : m_obj(obj) {
    INCREF(m_obj);
}

ReferenceCountedHardReference::~ReferenceCountedHardReference() {
    DECREF(m_obj);
}

}
