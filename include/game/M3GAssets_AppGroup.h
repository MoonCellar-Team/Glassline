#pragma once

#include "runtime/midp/array.h"

namespace m3g {
class Appearance;
}

class M3GAssets_AppGroup {
public:
    M3GAssets_AppGroup(int capacity, m3g::Appearance* defaultAppearance);
    ~M3GAssets_AppGroup();

    void addElement(int id, m3g::Appearance* appearance);
    void setDefaultAppearance(m3g::Appearance* appearance);
    void setMappedAppearance(m3g::Appearance* appearance, int index);
    void clearAppearances();
    m3g::Appearance* getAppearance(int id) const;
    int getNumMappings() const;

    midp::array<int> m_ids;
    midp::array<m3g::Appearance*> m_appearances;
    m3g::Appearance* m_defaultAppearance;
    int m_numMappings;
};
