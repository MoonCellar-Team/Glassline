#include "game/M3GAssets_AppGroup.h"

#include "m3g/Appearance.h"

void M3GAssets_AppGroup::setDefaultAppearance(m3g::Appearance* appearance) {
    midp::INCREF(appearance);
    midp::DECREF(m_defaultAppearance);
    m_defaultAppearance = appearance;
}

void M3GAssets_AppGroup::clearAppearances() {
    midp::DECREF(m_defaultAppearance);
    m_defaultAppearance = NULL;
    for (int i = 0; i < m_numMappings; i++) {
        midp::DECREF(m_appearances[i]);
        m_appearances[i] = NULL;
    }
}

void M3GAssets_AppGroup::setMappedAppearance(m3g::Appearance* appearance, int index) {
    midp::INCREF(appearance);
    midp::DECREF(m_appearances[index]);
    m_appearances[index] = appearance;
}

void M3GAssets_AppGroup::addElement(int id, m3g::Appearance* appearance) {
    m_ids[m_numMappings] = id;
    m_appearances[m_numMappings] = appearance;
    midp::INCREF(appearance);
    m_numMappings++;
}

m3g::Appearance* M3GAssets_AppGroup::getAppearance(int id) const {
    int count = m_numMappings;
    for (int i = 0; i < count; i++) {
        if (m_ids[i] == id) {
            return m_appearances[i];
        }
    }
    return m_defaultAppearance;
}

M3GAssets_AppGroup::~M3GAssets_AppGroup() {
    clearAppearances();
    m_numMappings = 0;
}

M3GAssets_AppGroup::M3GAssets_AppGroup(int capacity, m3g::Appearance* defaultAppearance)
    : m_ids(capacity),
      m_appearances(capacity),
      m_defaultAppearance(defaultAppearance),
      m_numMappings(0) {
    midp::INCREF(defaultAppearance);
}

int M3GAssets_AppGroup::getNumMappings() const {
    return m_numMappings;
}
