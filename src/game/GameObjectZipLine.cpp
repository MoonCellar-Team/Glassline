#include "game/GameObjectZipLine.h"

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/M3GAssets.h"
#include "game/SceneGame.h"
#include "m3g/Appearance.h"
#include "m3g/IndexBuffer.h"
#include "m3g/Mesh.h"
#include "m3g/VertexArray.h"
#include "m3g/VertexBuffer.h"
#include "runtime/midp/array.h"

static const int PRIMITIVE_LINES = 9;
static const int COMPONENT_FLOAT = 4;

GameObjectZipLine::~GameObjectZipLine() {}

GameObjectZipLine::GameObjectZipLine(MEdgeMap* map, MapPalette& palette, float x1, float y1, float x2, float y2,
                                     float endOffset)
    : GameObject(map, TYPE), m_endOffset(endOffset) {
    m3g::Group* world = AppEngine::getCanvas()->getSceneGame()->getM3GWorld();
    if (y2 < y1) {
        m_line.m_origin.set(x1, y1, 0.0f);
        m_line.m_direction.set(x2 - x1, y2 - y1, 0.0f);
    } else {
        m_line.m_origin.set(x2, y2, 0.0f);
        m_line.m_direction.set(x1 - x2, y1 - y2, 0.0f);
    }
    m_shape = new CollOrthoHexahedron(x1, y1, 0.0f, x2, y2, 0.0f);

    midp::array<float> noBias;
    m3g::IndexBuffer* indexBuffer;
    {
        const int lineIndices[2] = {0, 1};
        midp::array<int> indices(lineIndices, 2);
        indexBuffer = new m3g::IndexBuffer(PRIMITIVE_LINES, indices.m_length >> 1, indices);
    }
    float positions[6] = {x1, y1, 0.0f, x2, y2, 0.0f};
    m3g::VertexArray* vertexArray = new m3g::VertexArray(2, 3, COMPONENT_FLOAT);
    vertexArray->set(0, 2, positions);
    m3g::VertexBuffer* vertexBuffer = new m3g::VertexBuffer();
    vertexBuffer->setDefaultColor(0xffaa0000);
    vertexBuffer->setPositions(vertexArray, 1.0f, noBias);
    m3g::Appearance* appearance = AppEngine::getM3GAssets()->getAppearance(0);
    m3g::Mesh* mesh = new m3g::Mesh(vertexBuffer, indexBuffer, appearance);
    M3GAssets::commit(mesh);
    M3GAssets::addNode(world, mesh);
}

MathLine& GameObjectZipLine::getZipLineLine() {
    return m_line;
}

float GameObjectZipLine::getEndOffset() {
    return m_endOffset;
}
