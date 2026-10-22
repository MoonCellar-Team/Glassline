#pragma once
#include "m3g/IndexBuffer.h"

namespace m3g {

class TriangleStripArray : public IndexBuffer {
public:
    TriangleStripArray();
    TriangleStripArray(int first,const midp::array<int>& lengths);
    virtual ~TriangleStripArray();
    virtual IndexBuffer* commitDuplicate() const;
    virtual int getM3GUniqueClassID() const;
};

}
