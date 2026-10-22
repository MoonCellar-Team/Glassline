#include "game/UI/HorizontalSliderElement.h"

#include "game/QuadManager.h"
#include "runtime/midp/Graphics.h"

static const int SLIDER_HEIGHT = 40;
static const int GROUP_SLIDER = 0x35;
static const int MESH_KNOB = 0x7c;
static const int COLOR_TRACK = 0x7398b3;
static const int TRACK_THICKNESS = 4;
static const int SCOPE_UI = 2;
static const int ANCHOR_KNOB = QuadManager::ANCHOR_HCENTER | QuadManager::ANCHOR_VCENTER;

namespace UI {

bool HorizontalSliderElement::pointerPressed(int x, int y, int id) {
    m_value = (float)x / (float)m_width;
    m_dragging = true;
    return true;
}

bool HorizontalSliderElement::pointerReleased(int x, int y, int id) {
    m_value = (float)x / (float)m_width;
    m_dragging = false;
    return true;
}

bool HorizontalSliderElement::pointerDragged(int x, int y, int id) {
    m_value = (float)x / (float)m_width;
    return true;
}

void HorizontalSliderElement::render(midp::Graphics* g, int originY, int originX) {
    int centerY = m_y + originY + (m_height >> 1);
    g->setColor(COLOR_TRACK);
    g->fillRect(m_x + originX, centerY - 2, m_width, TRACK_THICKNESS);

    m_quadManager->setGroupVisible(GROUP_SLIDER, true);
    m_quadManager->setMeshVisible(MESH_KNOB, true);
    float x = (float)(m_x + originX) + m_value * (float)m_width;
    m_quadManager->setMeshPosition(MESH_KNOB, x, (float)centerY, ANCHOR_KNOB);
    m_quadManager->render(g, SCOPE_UI);
    m_quadManager->setMeshVisible(MESH_KNOB, false);
    m_quadManager->setGroupVisible(GROUP_SLIDER, false);
}

HorizontalSliderElement::HorizontalSliderElement(int width) : SliderElement(width, SLIDER_HEIGHT) {}

HorizontalSliderElement::~HorizontalSliderElement() {}

}
