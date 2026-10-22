#pragma once

#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class ChunkColumn;
class MapPalette;
class MathFrustum;
namespace m3g {
class Group;
}
namespace midp {
class DataInputStream;
}

class ChunkLayer : public midp::ReferenceCounted {
public:
    ChunkLayer(midp::DataInputStream& in, MapPalette& palette, m3g::Group* root);
    virtual ~ChunkLayer();

    void updateViewCulling(float dt, const MathFrustum& frustum);
    ChunkColumn* getColumn(int index);

    midp::array<midp::ReferenceCountedPointer<ChunkColumn> > m_columns;
    int m_unk1c;
    int m_columnCount;
    int m_unk24;
};
