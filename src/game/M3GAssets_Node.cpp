#include "game/M3GAssets_Node.h"

#include "game/M3GAssets.h"
#include "game/M3GAssets_File.h"
#include "m3g/Node.h"

M3GAssets_Node::M3GAssets_Node(M3GAssets_File* file, int userId, bool unique)
    : m_cacheFlags(0),
      m_file(file),
      m_userId(userId),
      m_node(NULL),
      m_unique(unique),
      m_modelLoads(0),
      m_modelRefs(0) {}

void M3GAssets_Node::free() {
    if (m_cacheFlags != 0 || m_modelLoads != 0) {
        return;
    }
    if (m_node == NULL) {
        return;
    }
    M3GAssets::orphanNode(m_node);
    midp::DECREF(m_node);
    m_node = NULL;
    m_file->free();
}

void M3GAssets_Node::freedFromModel() {
    m_modelLoads--;
    free();
}

void M3GAssets_Node::freeCached(int flags) {
    m_cacheFlags &= ~flags;
    free();
}

M3GAssets_Node::~M3GAssets_Node() {
    m_cacheFlags = 0;
    m_modelLoads = 0;
    free();
}

void M3GAssets_Node::load() {
    if (m_node != NULL) {
        return;
    }
    m_node = static_cast<m3g::Node*>(m_file->load());
    if (m_userId != -1) {
        m_node = static_cast<m3g::Node*>(m_node->find(m_userId));
    }
    midp::INCREF(m_node);
    M3GAssets::orphanNode(m_node);
}

m3g::Node* M3GAssets_Node::loadForModel() {
    m_modelLoads++;
    load();
    if (!m_unique && m_modelLoads == m_modelRefs) {
        return m_node;
    }
    return static_cast<m3g::Node*>(m_node->duplicate());
}

m3g::Node* M3GAssets_Node::loadUniqueCached(int flags) {
    m_cacheFlags |= flags;
    load();
    return static_cast<m3g::Node*>(m_node->duplicate());
}

m3g::Node* M3GAssets_Node::loadUnique() {
    load();
    m3g::Node* copy = static_cast<m3g::Node*>(m_node->duplicate());
    free();
    return copy;
}

void M3GAssets_Node::addModelRef() {
    m_modelRefs++;
}
