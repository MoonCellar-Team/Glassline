#include "game/M3GAssets_File.h"

#include "game/AppEngine.h"
#include "game/ResourceManager.h"
#include "m3g/Object3D.h"

M3GAssets_File::M3GAssets_File(int resId) : m_resId(resId), m_node(NULL), m_useCount(0) {}

M3GAssets_File::~M3GAssets_File() {
    midp::DECREF(m_node);
    m_node = NULL;
}

void M3GAssets_File::free() {
    int left = --m_useCount;
    if (m_node != NULL && left == 0) {
        midp::DECREF(m_node);
        m_node = NULL;
    }
}

m3g::Object3D* M3GAssets_File::load() {
    if (m_node == NULL) {
        m_node = AppEngine::getCanvas()->getResourceManager().loadM3GNode(m_resId);
    }
    m_useCount++;
    return m_node;
}
