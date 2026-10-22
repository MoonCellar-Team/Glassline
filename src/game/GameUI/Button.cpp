#include "game/GameUI/Button.h"

#include <stddef.h>

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameScreen.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/WindowStore.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"

namespace GameUI {

static const int FONT_LABEL = 8;
static const int FONT_LABEL_PRESSED = 7;
static const int LABEL_PADDING = 40;
static const int ANCHOR_CENTER = 0x12;
static const int MESH_ANCHOR = 9;
static const int QUAD_SCOPE = 2;

Button::Button(int group, int mesh, int labelId)
    : m_x(0),
      m_y(0),
      m_labelId(labelId),
      m_label(),
      m_group(group),
      m_mesh(mesh),
      m_width(0),
      m_pressed(false) {
    if (labelId == LABEL_NONE) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TextManager* text = canvas->getTextManager();
    m_label = text->getString(labelId)->toUpperCase();
    int meshWidth = quads.getMeshWidth(m_mesh);
    int textWidth = text->getStringWidth(m_label, FONT_LABEL);
    m_width = midp::Math::max(meshWidth, textWidth + LABEL_PADDING);
}

void Button::setTopLeft(int x, int y) {
    m_x = x;
    m_y = y;
}

bool Button::isPointerOnButton(int x, int y) {
    int height = getHeight();
    return x >= m_x && x <= m_x + m_width && y >= m_y && y <= m_y + height;
}

bool Button::pointerReleased(int pressX, int pressY, int x, int y, GameScreen* screen) {
    m_pressed = false;
    if (!isPointerOnButton(pressX, pressY) || !isPointerOnButton(x, y)) {
        return false;
    }
    int centerX = m_x + (m_width >> 1);
    int centerY = m_y + (getHeight() >> 1);
    int effectX = centerX;
    int effectY = centerY;
    if (screen != NULL) {
        screen->calcGlobalCoordinates(centerX, centerY, effectX, effectY);
    }
    AppEngine::getCanvas()->getWindowStore().getButtonEffect()->play(effectX, effectY);
    return true;
}

void Button::pointerDragged(int pressX, int pressY, int x, int y) {
    if (isPointerOnButton(pressX, pressY)) {
        m_pressed = isPointerOnButton(x, y);
    }
}

void Button::pointerPressed(int x, int y) {
    m_pressed = isPointerOnButton(x, y);
}

void Button::render2D(midp::Graphics* g) {
    if (m_label == NULL) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TextManager* text = canvas->getTextManager();
    int height = quads.getMeshHeight(m_mesh);
    text->drawString(g, m_label, m_pressed ? FONT_LABEL_PRESSED : FONT_LABEL, m_x + (m_width >> 1),
                     m_y + (height >> 1), ANCHOR_CENTER);
}

void Button::render3D(m3g::Graphics3D* g3d) {
    if (m_label == NULL) {
        return;
    }
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(m_group, true);
    int meshWidth = quads.getMeshWidth(m_mesh);
    int meshHeight = quads.getMeshHeight(m_mesh);
    quads.setMeshBounds(m_mesh, m_x, m_y, m_width, meshHeight, MESH_ANCHOR);
    quads.render(g3d, QUAD_SCOPE);
    quads.setGroupVisible(m_group, false);
    quads.setMeshBounds(m_mesh, 0.0f, 0.0f, meshWidth, meshHeight, MESH_ANCHOR);
}

int Button::getHeight() {
    return AppEngine::getCanvas()->getQuadManager().getMeshHeight(m_mesh);
}

Button::~Button() {}

int Button::getLabelId() {
    return m_labelId;
}

int Button::getWidth() {
    return m_width;
}

}
