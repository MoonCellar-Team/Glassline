#include "game/UI/UnlockableWindow.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/ResourceManager.h"
#include "game/TextManager.h"
#include "game/UI/UnlockableItem.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/String.h"

static const int TEXT_TITLE = 0x901;
static const int TEXT_SUBTITLE = 0x823;
static const int TEXT_SAVE = 0x902;
static const int FRAME_WIDTH = 924;
static const int FRAME_HEIGHT = 384;
static const int PANEL_WIDTH = 664;
static const int PANEL_Y_BIAS = 30;
static const int SAVE_GAP = 5;
static const int PHASE_MS = 300;
static const float PHASE_STEP = 0.0033333334f;
static const int GROUP_UNLOCK = 0x29;
static const int MESH_ANCHOR = QuadManager::ANCHOR_TOP | QuadManager::ANCHOR_LEFT;
static const int DESC_FONT = 2;
static const int DESC_Y = 0x20f;
static const int DESC_ANCHOR = 0x12;

static const int PHASE_IDLE = 0;
static const int PHASE_IN = 1;
static const int PHASE_SHOWN = 2;
static const int PHASE_OUT = 3;

static float truncToFloat(float value) {
    return static_cast<float>(static_cast<int>(value));
}

namespace UI {

void UnlockableWindow::displayUnlockable(UnlockableItem* item) {
    m_phase = PHASE_IN;
    m_timer = PHASE_MS;
    m_item = item;
    m_saved = false;
}

bool UnlockableWindow::pointerPressed(int x, int y, int id) {
    if (m_phase != PHASE_IDLE) {
        return false;
    }
    TitledWindow::pointerPressed(x, y, id);
    if (m_save.contains(x, y)) {
        m_save.pointerPressed(x, y, id);
        return false;
    }
    if (m_panel.contains(x, y)) {
        m_panel.pointerPressed(m_panel.toRelativeX(x), y, id);
        return true;
    }
    return false;
}

void UnlockableWindow::update(int dt) {
    TitledWindow::update(dt);
    if (m_save.isActivated() && !m_saved) {
        m_save.reset();
        m_saved = true;
        m_item->saveToLibrary();
        AppEngine::getCanvas()->startFadeIn(true);
    }
    if (m_phase == PHASE_OUT) {
        if (!AppEngine::getCanvas()->isFading()) {
            m_timer -= dt;
            if (m_timer <= 0) {
                m_phase = PHASE_IDLE;
                m_timer = 0;
            }
        }
        return;
    }
    if (m_phase == PHASE_IN) {
        m_timer -= dt;
        if (m_timer <= 0) {
            m_phase = PHASE_SHOWN;
            m_timer = 0;
        }
        return;
    }
    if (m_phase == PHASE_IDLE) {
        m_panel.update(dt);
    }
}

void UnlockableWindow::render(midp::Graphics* g, int originY, int originX) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TitledWindow::render(g, originY, originX);
    m_panel.render(g, originY, originX);
    UnlockableItem* selected = static_cast<UnlockableItem*>(m_panel.getSelectedItem());
    if (selected != NULL && !selected->isUnlocked()) {
        TextManager* text = canvas->getTextManager();
        midp::ReferenceCountedPointer<midp::String> description = selected->getDescription();
        text->drawString(g, description, DESC_FONT, m_width >> 1, DESC_Y, DESC_ANCHOR);
    }
    m_right.render(g, originY, originX);
    m_left.render(g, originY, originX);
    if ((unsigned)(m_phase - 1) >= 3u) {
        return;
    }
    float t = static_cast<float>(m_timer) * PHASE_STEP;
    if (m_phase != PHASE_OUT) {
        t = 1.0f - t;
    }
    int thumbWidth = m_item->getThumbWidth();
    int thumbHeight = m_item->getThumbHeight();
    int thumbX = m_item->getThumbX();
    int thumbY = m_item->getThumbY();
    float width = static_cast<float>(thumbWidth) + static_cast<float>(m_width - thumbWidth) * t;
    float height = static_cast<float>(thumbHeight) + static_cast<float>(m_height - thumbHeight) * t;
    float x = (1.0f - t) * static_cast<float>(thumbX);
    float y = (1.0f - t) * static_cast<float>(thumbY);
    int mesh = m_item->getQuadId();
    quads.setMeshBounds(mesh, truncToFloat(x), truncToFloat(y), truncToFloat(width), truncToFloat(height),
                        MESH_ANCHOR);
    quads.setGroupVisible(GROUP_UNLOCK, true);
    quads.setMeshVisible(mesh, true);
    quads.render(g);
    quads.setMeshVisible(mesh, false);
    quads.setGroupVisible(GROUP_UNLOCK, false);
    if (canvas->isFading()) {
        return;
    }
    m_backButton.render(g, originY, originX);
    if (m_phase == PHASE_SHOWN) {
        m_save.render(g, originY, originX);
    }
}

bool UnlockableWindow::pointerReleased(int x, int y, int id) {
    if (m_phase == PHASE_SHOWN) {
        if (m_save.contains(x, y)) {
            m_save.pointerReleased(x, y, id);
        } else if (!m_backButton.contains(x, y)) {
            return true;
        }
        m_phase = PHASE_OUT;
        m_timer = PHASE_MS;
        return true;
    }
    if (m_phase != PHASE_IDLE) {
        return false;
    }
    TitledWindow::pointerReleased(x, y, id);
    if (m_panel.contains(x, y)) {
        m_panel.pointerReleased(x, y, id);
        return true;
    }
    if (m_right.contains(x, y)) {
        m_panel.next();
        m_right.pointerReleased(x, y, id);
        return false;
    }
    if (m_left.contains(x, y)) {
        m_panel.prev();
        m_left.pointerReleased(x, y, id);
        return false;
    }
    return false;
}

bool UnlockableWindow::pointerDragged(int x, int y, int id) {
    if (m_phase != PHASE_IDLE) {
        return false;
    }
    TitledWindow::pointerDragged(x, y, id);
    if (!m_save.contains(x, y) && m_save.isPressed()) {
        m_save.unpress();
        return false;
    }
    if (m_panel.contains(x, y)) {
        m_panel.pointerDragged(m_panel.toRelativeX(x), y, id);
        return true;
    }
    if (m_panel.isDragging()) {
        m_panel.pointerReleased(x, y, id);
        return false;
    }
    return false;
}

UnlockableWindow::UnlockableWindow()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE),
      m_phase(PHASE_IDLE),
      m_timer(0),
      m_item(NULL),
      m_panel(this),
      m_save(TEXT_SAVE),
      m_saved(false) {
    int frameX = (m_width - FRAME_WIDTH) >> 1;
    int frameY = (m_height - FRAME_HEIGHT) >> 1;
    m_background.setDimensions(FRAME_WIDTH, FRAME_HEIGHT);
    m_background.setPosition(frameX, frameY);
    m_panel.setPosition((m_width - PANEL_WIDTH) >> 1, frameY + PANEL_Y_BIAS);
    m_save.setPosition((m_backButton.getX() - SAVE_GAP) - m_save.getWidth(), m_backButton.getY());
    int arrowY = m_panel.getY() + ((m_panel.getHeight() - m_right.getHeight()) >> 1);
    int leftX = ((m_panel.getX() - frameX - m_left.getWidth()) >> 1) + frameX;
    m_right.setPosition((m_width - m_right.getWidth()) - leftX, arrowY);
    m_left.setPosition(leftX, arrowY);

    midp::ReferenceCountedPointer<midp::InputStream> file(
        midp::Class::getResourceAsStream(new midp::String("unlockables.bin")));
    if (file != NULL) {
        midp::DataInputStream in(file);
        int count = in.readInt();
        while (count > 0) {
            int quad = ResourceManager::UNLOCKABLE_QUAD_DATA[in.readShort()];
            int image = ResourceManager::UNLOCKABLE_SAVEABLE_DATA[in.readShort()];
            int text = ResourceManager::UNLOCKABLE_STRING_DATA[in.readShort()];
            int badges = in.readInt();
            m_panel.addItem(new UnlockableItem(quad, image, text, badges));
            count -= 1;
        }
    }
}

UnlockableWindow::~UnlockableWindow() {}

}
