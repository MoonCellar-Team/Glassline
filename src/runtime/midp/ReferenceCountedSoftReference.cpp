#include "runtime/midp/ReferenceCountedSoftReference.h"

namespace midp {

ReferenceCountedSoftReference::ReferenceCountedSoftReference(const ReferenceCounted* object) : m_object(object) {
    INCREF(const_cast<ReferenceCounted*>(m_object));
}

ReferenceCountedSoftReference::~ReferenceCountedSoftReference() {
    DECREF_NO_DELETE(const_cast<ReferenceCounted*>(m_object));
}

}
