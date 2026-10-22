#include "game/QuadGroup.h"

#include <stddef.h>

QuadGroup::QuadGroup()
    : m_group(NULL),
      m_parent(-1),
      m_visible(true),
      m_firstGroup(0),
      m_numGroups(0),
      m_firstMesh(0),
      m_numMeshes(0),
      m_x(0),
      m_y(0) {}
