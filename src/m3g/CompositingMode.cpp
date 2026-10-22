#include "m3g/CompositingMode.h"
#include "m3g/CompositingModeClass.h"

namespace m3g {

CompositingMode::CompositingMode() : m_blending(0x44), m_blender(nullptr), m_alphaThreshold(0), m_depthTest(true), m_depthWrite(true), m_colorWrite(true), m_alphaWrite(true), m_depthOffsetFactor(0), m_depthOffsetUnits(0) {}

CompositingMode::~CompositingMode() { midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_blender)); }

midp::Class* CompositingMode::getClass() const { return new CompositingModeClass; }

int CompositingMode::getM3GUniqueClassID() const { return 6; }

void CompositingMode::setBlender(Blender* blender) {
    midp::INCREF(reinterpret_cast<midp::ReferenceCounted*>(blender));
    midp::DECREF(reinterpret_cast<midp::ReferenceCounted*>(m_blender));
    m_blender = blender;
}

Blender* CompositingMode::getBlender() const { return m_blender; }

void CompositingMode::setAlphaThreshold(float value) { setAlphaThresholdx(static_cast<int>(value * 65536.0f)); }

float CompositingMode::getAlphaThreshold() const { return static_cast<float>(getAlphaThresholdx()) * 0.0000152587890625f; }

void CompositingMode::setDepthOffsetx(int factor, int units) { m_depthOffsetFactor = factor; m_depthOffsetUnits = units; }

void CompositingMode::setDepthOffset(float factor, float units) { setDepthOffsetx(static_cast<int>(factor * 65536.0f), static_cast<int>(units * 65536.0f)); }

void CompositingMode::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target);
    CompositingMode* result = static_cast<CompositingMode*>(target);
    result->setBlending(getBlending()); result->setBlender(getBlender());
    result->setAlphaThreshold(getAlphaThreshold());
    result->setColorWriteEnable(isColorWriteEnabled());
    result->setDepthWriteEnable(isDepthWriteEnabled());
    result->setDepthTestEnable(isDepthTestEnabled());
    int factor = getDepthOffsetFactorx(); int units = getDepthOffsetUnitsx();
    result->setDepthOffsetx(factor, units);
}

void CompositingMode::setBlending(int value) { m_blending = value; }

int CompositingMode::getBlending() const { return m_blending; }

void CompositingMode::setAlphaThresholdx(int value) { m_alphaThreshold = value; }

int CompositingMode::getAlphaThresholdx() const { return m_alphaThreshold; }

void CompositingMode::setAlphaWriteEnable(bool value) { m_alphaWrite = value; }

bool CompositingMode::isAlphaWriteEnabled() const { return m_alphaWrite; }

void CompositingMode::setColorWriteEnable(bool value) { m_colorWrite = value; }

bool CompositingMode::isColorWriteEnabled() const { return m_colorWrite; }

void CompositingMode::setDepthWriteEnable(bool value) { m_depthWrite = value; }

bool CompositingMode::isDepthWriteEnabled() const { return m_depthWrite; }

void CompositingMode::setDepthTestEnable(bool value) { m_depthTest = value; }

bool CompositingMode::isDepthTestEnabled() const { return m_depthTest; }

int CompositingMode::getDepthOffsetFactorx() const { return m_depthOffsetFactor; }

float CompositingMode::getDepthOffsetFactor() const { return static_cast<float>(getDepthOffsetFactorx()) * 0.0000152587890625f; }

int CompositingMode::getDepthOffsetUnitsx() const { return m_depthOffsetUnits; }

float CompositingMode::getDepthOffsetUnits() const { return static_cast<float>(getDepthOffsetUnitsx()) * 0.0000152587890625f; }

}
