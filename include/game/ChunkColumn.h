#pragma once

#include "game/MathOrthoBox.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class ChunkSection;
class GameObjectPlayer;
class MapPalette;
class MathFrustum;
namespace m3g {
class Group;
}
namespace midp {
class DataInputStream;
}

class ChunkColumn : public midp::ReferenceCounted {
public:
    ChunkColumn(midp::DataInputStream& in, MapPalette& palette);
    virtual ~ChunkColumn();

    void update(float dt, const MathFrustum& frustum, GameObjectPlayer* player);
    MathOrthoBox& getBounds();
    unsigned int& getCoherencyIndex();
    ChunkSection* getSection(int index);
    m3g::Group* getColumnGroup();

    MathOrthoBox m_bounds;
    midp::array<midp::ReferenceCountedPointer<ChunkSection> > m_sections;
    midp::ReferenceCountedPointer<m3g::Group> m_group;
    int m_unk3c;
    int m_sectionCount;
    int m_unk44;
    unsigned int m_coherencyIndex;
};
