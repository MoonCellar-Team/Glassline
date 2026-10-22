#include "m3g/Fog.h"

namespace m3g {

int Fog::getColor() const { return m_color; }

float Fog::getDensity() const { return m_density; }

float Fog::getFarDistance() const { return m_farDistance; }

int Fog::getMode() const { return m_mode; }

float Fog::getNearDistance() const { return m_nearDistance; }

}
