#include "game/QuadMesh.h"

#include <stddef.h>

#include "m3g/Mesh.h"

QuadMesh::QuadMesh()
    : m_node(NULL),
      m_vertexBuffer(NULL),
      m_group(0),
      m_texture(0),
      m_textureEntry(NULL),
      m_scope(-1),
      m_layer(0),
      m_color(-1),
      m_x(0),
      m_y(0),
      m_visible(true),
      m_dirty(false) {}

void QuadMesh::setVisible(bool visible) {
    if (m_node != NULL) {
        m_node->setRenderingEnable(visible);
    }
}
