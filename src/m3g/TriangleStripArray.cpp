#include "m3g/TriangleStripArray.h"

namespace m3g {

TriangleStripArray::TriangleStripArray() { setPrimitiveType(8); }

TriangleStripArray::TriangleStripArray(int first,const midp::array<int>& lengths) : IndexBuffer(8,lengths,first) {}

TriangleStripArray::~TriangleStripArray() {}

int TriangleStripArray::getM3GUniqueClassID() const { return 11; }

IndexBuffer* TriangleStripArray::commitDuplicate() const {
    int count=getIndexCount(); midp::array<int> indices(count); getIndices(indices);
    int primitives=getPrimitiveCount(); return new IndexBuffer(8,primitives,indices);
}

}
