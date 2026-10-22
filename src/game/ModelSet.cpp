#include "game/ModelSet.h"

int ModelSet::getBackgroundFillColor() const {
    return m_backgroundFillColor;
}

int ModelSet::getUserId(int index) const {
    return m_userIds[index];
}

int ModelSet::getModelId(int index) const {
    return m_modelIds[index];
}

float ModelSet::getRotateY() const {
    return m_rotateY;
}

float ModelSet::getRotateZ() const {
    return m_rotateZ;
}

ModelSet::ModelSet()
    : m_modelIds(NUM_MODELS), m_userIds(NUM_MODELS), m_backgroundFillColor(0), m_rotateY(0), m_rotateZ(0) {}

ModelSet::~ModelSet() {}

ModelSet::ModelSet(const ModelSet& other)
    : m_modelIds(other.m_modelIds),
      m_userIds(other.m_userIds),
      m_backgroundFillColor(other.m_backgroundFillColor),
      m_rotateY(other.m_rotateY),
      m_rotateZ(other.m_rotateZ) {}
