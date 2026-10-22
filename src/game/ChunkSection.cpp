#include "game/ChunkSection.h"

#include "game/ChunkRunnerVision.h"
#include "game/GameObjectPlayer.h"
#include "game/M3GAssets.h"
#include "game/MapPalette.h"
#include "game/MathVector.h"
#include "m3g/Node.h"
#include "runtime/midp/DataInputStream.h"

#ifdef MEDECOMP_PORT
#include <map>

#include "runtime/midp/ReferenceCountedPointer.h"

extern "C" int medecomp_player_level_playing(void);
static const int PIECE_MIRROR_BIT = 0x40000000;
void medecomp_mirror_piece(m3g::Node* node);
#endif

static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;

static const float MIN_DEPTH = 10.0f;

m3g::Node* ChunkSection::getChunkNode(int index) {
    m3g::Node* node = m_node;
    if (m_nodeCount == 1) {
        return node;
    }
    return static_cast<m3g::Group*>(node)->getChild(index);
}

ChunkSection::~ChunkSection() {}

#ifdef MEDECOMP_PORT
static std::map<int, midp::ReferenceCountedPointer<m3g::Node> > s_playerTemplates;

void medecomp_chunk_load_begin() {
    s_playerTemplates.clear();
}

static m3g::Node* playerNode(MapPalette& palette, int userId, bool shared) {
    std::map<int, midp::ReferenceCountedPointer<m3g::Node> >::iterator given = s_playerTemplates.find(userId);
    if (given != s_playerTemplates.end()) {
        return static_cast<m3g::Node*>(given->second->duplicate());
    }
    if (!shared) {
        return palette.createUniqueNode(userId);
    }
    m3g::Node* node = palette.getNode(userId);
    if (node != NULL) {
        s_playerTemplates[userId] = static_cast<m3g::Node*>(node->duplicate());
    }
    return node;
}

m3g::Node* medecomp_piece_template(MapPalette& palette, int userId) {
    std::map<int, midp::ReferenceCountedPointer<m3g::Node> >::iterator given = s_playerTemplates.find(userId);
    if (given != s_playerTemplates.end()) return given->second;
    return palette.getNode(userId);
}
#endif

m3g::Node* ChunkSection::readNode(midp::DataInputStream& in, MapPalette& palette) {
    int userId = in.readInt();
    float x = in.readInt() * FIXED_TO_FLOAT;
    float y = in.readInt() * FIXED_TO_FLOAT;
    float z = in.readInt() * FIXED_TO_FLOAT;
    m3g::Node* node;
    bool shared = in.readBoolean();
#ifdef MEDECOMP_PORT
    const bool mirror = (userId & PIECE_MIRROR_BIT) != 0 && medecomp_player_level_playing();
    if (mirror) userId &= ~PIECE_MIRROR_BIT;
    if (medecomp_player_level_playing()) {
        node = playerNode(palette, userId, shared);
    } else
#endif
    if (shared) {
        node = palette.getNode(userId);
    } else {
        node = palette.createUniqueNode(userId);
    }
#ifdef MEDECOMP_PORT
    if (node == NULL) node = new m3g::Group();
#endif
    node->translate(x, y, z);
#ifdef MEDECOMP_PORT
    if (mirror) medecomp_mirror_piece(node);
#endif
    return node;
}

ChunkSection::ChunkSection(midp::DataInputStream& in, MapPalette& palette)
    : m_nodeCount(0), m_coherencyIndex((unsigned int)-1) {
    m_bounds.setF(&in);
    m_runnerVision = midp::array<midp::ReferenceCountedPointer<ChunkRunnerVision> >(in.readByte());
    m_nodeCount = in.readByte();
    m3g::Group* group = NULL;
    if (m_nodeCount > 1) {
        group = new m3g::Group();
        m_node = group;
    }
    int runnerVisionCount = 0;
    for (int i = 0; i != m_nodeCount; i++) {
        m3g::Node* node = readNode(in, palette);
        if (in.readByte() == 1) {
            m_runnerVision[runnerVisionCount++] = new ChunkRunnerVision(in, node);
        }
        if (m_nodeCount == 1) {
            m_node = node;
        } else {
            M3GAssets::addNode(group, node);
        }
    }
    if (m_bounds.m_min.z - m_bounds.m_max.z > -MIN_DEPTH) {
        m_bounds.m_min.z = m_bounds.m_max.z - MIN_DEPTH;
    }
}

void ChunkSection::updateRunnerVision(float dt, GameObjectPlayer* player) {
    m_node->setRenderingEnable(true);
    int count = m_runnerVision.m_length;
    if (count == 0 || player == NULL) {
        return;
    }
    MathVector position(player->m_position);
    int facing = player->getFacingDir();
    for (int i = 0; i != count; i++) {
        m_runnerVision[i]->updateIntensity(dt, position, facing);
    }
}

MathOrthoBox& ChunkSection::getBounds() {
    return m_bounds;
}

unsigned int& ChunkSection::getCoherencyIndex() {
    return m_coherencyIndex;
}

m3g::Node* ChunkSection::getSectionNode() {
    return m_node;
}

ChunkRunnerVision* ChunkSection::getRunnerVisionChunk(int index) {
    midp::ReferenceCountedPointer<ChunkRunnerVision> chunk =
        index < m_runnerVision.m_length ? m_runnerVision[index]
                                        : midp::ReferenceCountedPointer<ChunkRunnerVision>(NULL);
    return chunk;
}
