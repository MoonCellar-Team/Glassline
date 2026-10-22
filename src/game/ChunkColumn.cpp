#include "game/ChunkColumn.h"

#include "game/ChunkSection.h"
#include "game/M3GAssets.h"
#include "game/MathFrustum.h"
#include "m3g/Node.h"
#include "runtime/midp/DataInputStream.h"

static const int FRUSTUM_OUTSIDE = -1;

ChunkColumn::~ChunkColumn() {}

ChunkColumn::ChunkColumn(midp::DataInputStream& in, MapPalette& palette)
    : m_unk3c(0), m_sectionCount(0), m_unk44(0), m_coherencyIndex((unsigned int)-1) {
    m_bounds.setF(&in);
    int count = in.readByte();
    m_sections = midp::array<midp::ReferenceCountedPointer<ChunkSection> >(count);
    m_sectionCount = count;
    for (int i = 0; i != count; i++) {
        m_sections[i] = new ChunkSection(in, palette);
    }
    m3g::Group* group = new m3g::Group();
    m_group = group;
    for (int i = 0; i != count; i++) {
        M3GAssets::addNode(group, m_sections[i]->getSectionNode());
    }
}

void ChunkColumn::update(float dt, const MathFrustum& frustum, GameObjectPlayer* player) {
    m_group->setRenderingEnable(true);
    int count = m_sections.m_length;
    if (count == 0) {
        return;
    }
    if (count == 1) {
        m_sections[0]->updateRunnerVision(dt, player);
        return;
    }
    for (int i = 0; i != count; i++) {
        MathOrthoBox& bounds = m_sections[i]->getBounds();
        if (frustum.intersectAABBCoherency(bounds.m_min, bounds.m_max, m_sections[i]->getCoherencyIndex()) ==
            FRUSTUM_OUTSIDE) {
            m_sections[i]->getSectionNode()->setRenderingEnable(false);
        } else {
            m_sections[i]->updateRunnerVision(dt, player);
        }
    }
}

MathOrthoBox& ChunkColumn::getBounds() {
    return m_bounds;
}

unsigned int& ChunkColumn::getCoherencyIndex() {
    return m_coherencyIndex;
}

ChunkSection* ChunkColumn::getSection(int index) {
    return m_sections[index];
}

m3g::Group* ChunkColumn::getColumnGroup() {
    return m_group;
}
