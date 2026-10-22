#include "m3g/PolygonMode.h"
#include "m3g/PolygonModeClass.h"

namespace m3g {

PolygonMode::PolygonMode() : m_culling(0xa0), m_winding(0xa8), m_shading(0xa5), m_twoSided(false), m_localCamera(false), m_perspective(false) {}

PolygonMode::~PolygonMode() {}

midp::Class* PolygonMode::getClass() const { return new PolygonModeClass; }

int PolygonMode::getM3GUniqueClassID() const { return 8; }

void PolygonMode::duplicateTo(Object3D* target) {
    Object3D::duplicateTo(target);
    PolygonMode* result = static_cast<PolygonMode*>(target);
    result->setCulling(getCulling()); result->setWinding(getWinding()); result->setShading(getShading());
    result->setTwoSidedLightingEnable(isTwoSidedLightingEnabled());
    result->setLocalCameraLightingEnable(isLocalCameraLightingEnabled());
    result->setPerspectiveCorrectionEnable(isPerspectiveCorrectionEnabled());
}

int PolygonMode::getCulling() const { return m_culling; }

void PolygonMode::setCulling(int value) { m_culling = value; }

int PolygonMode::getWinding() const { return m_winding; }

void PolygonMode::setWinding(int value) { m_winding = value; }

int PolygonMode::getShading() const { return m_shading; }

void PolygonMode::setShading(int value) { m_shading = value; }

bool PolygonMode::isTwoSidedLightingEnabled() const { return m_twoSided; }

void PolygonMode::setTwoSidedLightingEnable(bool value) { m_twoSided = value; }

bool PolygonMode::isLocalCameraLightingEnabled() const { return m_localCamera; }

void PolygonMode::setLocalCameraLightingEnable(bool value) { m_localCamera = value; }

bool PolygonMode::isPerspectiveCorrectionEnabled() const { return m_perspective; }

void PolygonMode::setPerspectiveCorrectionEnable(bool value) { m_perspective = value; }

}
