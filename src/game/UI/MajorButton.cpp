#include "game/UI/MajorButton.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"

static const int GROUP_BUTTON = 0x45;
static const int MESH_BUTTON = 0x88;
static const int MESH_ANCHOR = QuadManager::ANCHOR_TOP | QuadManager::ANCHOR_LEFT;
static const int RENDER_SCOPE = 2;
static const int TEXT_ANCHOR = 18;
static const int TEXT_PAD_X = 0x28;
static const int NO_TEXT = -1;
#ifdef MEDECOMP_PORT
static const int DISABLED_FACE = int(0xFF959DA8u);
static const int DISABLED_SHADOW = int(0xFF5F6670u);
static const int DISABLED_SHADOW_X = 4, DISABLED_SHADOW_Y = 7;
#endif

namespace UI {

void MajorButton::render(midp::Graphics* g, int originY, int originX) {
    QuadManager* quads = m_quadManager;
    quads->setGroupVisible(GROUP_BUTTON, true);
    int savedWidth = quads->getMeshWidth(MESH_BUTTON);
    int savedHeight = quads->getMeshHeight(MESH_BUTTON);
    quads->setMeshBounds(MESH_BUTTON, static_cast<float>(m_x + originX), static_cast<float>(m_y + originY),
                         static_cast<float>(m_width), static_cast<float>(m_height), MESH_ANCHOR);
#ifdef MEDECOMP_PORT
    if (!m_enabled) {
        int x = m_x + originX, y = m_y + originY;
        g->setColor(DISABLED_SHADOW);
        g->fillRect(x + DISABLED_SHADOW_X, y + DISABLED_SHADOW_Y, m_width - DISABLED_SHADOW_X, m_height - DISABLED_SHADOW_Y);
        g->setColor(DISABLED_FACE);
        g->fillRect(x, y, m_width - DISABLED_SHADOW_X, m_height - DISABLED_SHADOW_Y);
    } else {
        quads->render(g, RENDER_SCOPE);
    }
#else
    quads->render(g, RENDER_SCOPE);
#endif

    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int language = text->getCurrentLanguage();
    if (language != m_language) {
        m_language = text->getCurrentLanguage();
        m_label = text->getString(m_text)->toUpperCase();
    }
    int font = m_pressed ? m_unk24 : m_font;
    int x = originX + m_x + (m_width >> 1) - 3;
    int y = originY + m_y + (m_height >> 1) - 1;
    text->drawString(g, m_label, font, x, y, TEXT_ANCHOR);

    quads->setGroupVisible(GROUP_BUTTON, false);
    quads->setMeshBounds(MESH_BUTTON, 0.0f, 0.0f, static_cast<float>(savedWidth),
                         static_cast<float>(savedHeight), MESH_ANCHOR);
}

MajorButton::MajorButton(int text)
    : Button(text),
      m_language(0),
      m_label(NULL) {
    TextManager* labels = AppEngine::getCanvas()->getTextManager();
    m_language = labels->getCurrentLanguage();
    int width;
    int height;
    if (text == NO_TEXT) {
        width = m_quadManager->getMeshWidth(MESH_BUTTON);
        height = m_quadManager->getMeshHeight(MESH_BUTTON);
    } else {
        m_label = labels->getString(m_text)->toUpperCase();
        int meshWidth = m_quadManager->getMeshWidth(MESH_BUTTON);
        height = m_quadManager->getMeshHeight(MESH_BUTTON);
        int textWidth = labels->getStringWidth(m_text, m_font);
        width = textWidth + TEXT_PAD_X;
        if (width <= meshWidth) {
            width = meshWidth;
        }
    }
    setWidth(width);
    setHeight(height);
}

MajorButton::~MajorButton() {}

}
