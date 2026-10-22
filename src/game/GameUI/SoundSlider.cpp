#include "game/GameUI/SoundSlider.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/Math.h"

namespace GameUI {

static const int FONT_LABEL = 7;
static const int LABEL_ANCHOR = 0x11;
static const int GROUP_SLIDER = 0x35;
static const int MESH_KNOB = 0x7c;
static const int MESH_KNOB_OFF = 0x7d;
static const int MESH_LOW = 0x7e;
static const int MESH_HIGH = 0x7f;
static const int ICON_GAP = 0x28;
static const int HIT_SLOP = 5;
static const int ANCHOR_KNOB = QuadManager::ANCHOR_HCENTER | QuadManager::ANCHOR_VCENTER;
static const int ANCHOR_LOW = QuadManager::ANCHOR_VCENTER | QuadManager::ANCHOR_RIGHT;
static const int ANCHOR_HIGH = QuadManager::ANCHOR_VCENTER | QuadManager::ANCHOR_LEFT;
static const int TRACK_COLOR = 0x7398b3;
static const int QUAD_SCOPE = 2;

float SoundSlider::getValue() {
    return m_value;
}

void SoundSlider::setValue(float value) {
    m_value = value;
}

int SoundSlider::getLabelWidth() {
    return m_labelWidth;
}

int SoundSlider::getHeight() {
    return m_height;
}

void SoundSlider::setY(float y) {
    m_y = (int)y;
}

bool SoundSlider::isPressed() {
    return m_pointerId != -1;
}

SoundSlider::SoundSlider(int labelId, float maxValue)
    : m_labelX(0),
      m_labelWidth(0),
      m_left(0),
      m_right(0),
      m_y(0),
      m_height(0),
      m_value(0.0f),
      m_max(maxValue),
      m_labelId(labelId),
      m_track(TRACK_COLOR),
      m_pointerId(-1) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TextManager* text = canvas->getTextManager();
    m_labelWidth = text->getStringWidth(labelId, FONT_LABEL);
    m_height = midp::Math::max(text->getLineHeight(FONT_LABEL), quads.getMeshHeight(MESH_KNOB));
}

SoundSlider::~SoundSlider() {}

int SoundSlider::pointerDragged(int x, int y, int id) {
    (void)y;
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    float span = (float)(m_right - m_left);
    float knob = (float)m_left + span * (m_value / m_max);
    quads.setMeshPosition(MESH_KNOB, (float)(int)knob, (float)m_y, ANCHOR_KNOB);
    if (m_pointerId != id) {
        return 0;
    }
    float ratio = (float)(x - m_left) / span;
    float next = midp::Math::max(0.0f, midp::Math::min(m_max, m_max * ratio));
    if (m_value == next) {
        return 0;
    }
    m_value = midp::Math::max(0.0f, midp::Math::min(m_max, next));
    return 1;
}

bool SoundSlider::pointerReleased(int id) {
    if (m_pointerId != id) {
        return false;
    }
    m_pointerId = -1;
    return true;
}

void SoundSlider::setCoordinates(int labelX, int trackLeft, int trackRight) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    m_labelX = labelX;
    m_left = trackLeft + quads.getMeshWidth(MESH_LOW) + ICON_GAP;
    m_right = trackRight - ICON_GAP - quads.getMeshWidth(MESH_HIGH);
}

void SoundSlider::pointerPressed(int x, int y, int id) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    float span = (float)(m_right - m_left);
    float knob = (float)m_left + span * (m_value / m_max);
    int knobX = (int)knob;
    int halfW = (quads.getMeshWidth(MESH_KNOB) >> 1) + HIT_SLOP;
    int halfH = (quads.getMeshHeight(MESH_KNOB) >> 1) + HIT_SLOP;
    if (knobX - halfW <= x && x <= knobX + halfW && m_y - halfH <= y && y <= m_y + halfH) {
        m_pointerId = id;
    }
}

void SoundSlider::render2D(midp::Graphics* g) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, m_labelId, FONT_LABEL, m_labelX, m_y, LABEL_ANCHOR);
}

void SoundSlider::render3D(m3g::Graphics3D* g3d, bool enabled) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int span = m_right - m_left;
    m_track.setBounds(m_left, m_y - 2, span, 4);
    m_track.render3D(g3d);
    quads.setGroupVisible(GROUP_SLIDER, true);
    quads.setMeshVisible(MESH_KNOB, enabled);
    quads.setMeshVisible(MESH_KNOB_OFF, !enabled);
    quads.setMeshVisible(MESH_LOW, true);
    quads.setMeshVisible(MESH_HIGH, true);
    float knob = (float)m_left + (float)span * (m_value / m_max);
    float y = (float)m_y;
    quads.setMeshPosition(MESH_LOW, (float)(m_left - ICON_GAP), y, ANCHOR_LOW);
    quads.setMeshPosition(MESH_HIGH, (float)(m_right + ICON_GAP), y, ANCHOR_HIGH);
    float knobX = (float)(int)knob;
    quads.setMeshPosition(MESH_KNOB, knobX, y, ANCHOR_KNOB);
    quads.setMeshPosition(MESH_KNOB_OFF, knobX, y, ANCHOR_KNOB);
    quads.render(g3d, QUAD_SCOPE);
    quads.setMeshVisible(MESH_HIGH, false);
    quads.setMeshVisible(MESH_LOW, false);
    quads.setMeshVisible(MESH_KNOB_OFF, false);
    quads.setMeshVisible(MESH_KNOB, false);
    quads.setGroupVisible(GROUP_SLIDER, false);
}

}
