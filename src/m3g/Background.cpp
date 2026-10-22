#include "m3g/Background.h"

namespace m3g {

Background::Background() : m_color(0), m_colorClear(true), m_depthClear(true) {}

Background::~Background() {}

int Background::getM3GUniqueClassID() const { return 4; }

int Background::getColor() const { return m_color; }

bool Background::isColorClearEnabled() const { return m_colorClear; }

bool Background::isDepthClearEnabled() const { return m_depthClear; }

void Background::setColor(int value) { m_color = value; }

void Background::setColorClearEnable(bool value) { m_colorClear = value; }

void Background::setDepthClearEnable(bool value) { m_depthClear = value; }

}
