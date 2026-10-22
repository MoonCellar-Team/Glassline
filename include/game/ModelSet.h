#pragma once

#include "runtime/midp/array.h"

class ModelSet {
public:
    static const int NUM_MODELS = 2;

    ModelSet();
    ModelSet(const ModelSet& other);
    ~ModelSet();

    int getModelId(int index) const;
    int getUserId(int index) const;
    int getBackgroundFillColor() const;
    float getRotateY() const;
    float getRotateZ() const;

    midp::array<int> m_modelIds;
    midp::array<int> m_userIds;
    int m_backgroundFillColor;
    float m_rotateY;
    float m_rotateZ;
};
