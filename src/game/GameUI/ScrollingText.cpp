#include "game/GameUI/ScrollingText.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameScreen.h"
#include "game/TextManager.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"

namespace GameUI {

static const int TEXT_NONE = 0x800;
static const int TEXT_BACK = 0x82f;
static const int FONT_TITLE = 16;
static const int FONT_HEADER = 17;
static const int FONT_BODY = 2;
static const int ANCHOR_LEFT_TOP = 9;
static const int ANCHOR_LEFT_BASELINE = 0x41;
static const int COLOR_THUMB = 0xfffe0000;
static const int COLOR_TRACK = 0xff8794a4;

ScrollingText::ScrollingText(SceneGame* scene, GameScreen* screen, int titleId, int headerId, int textId)
    : m_scene(scene),
      m_screen(screen),
      m_title(),
      m_header(),
      m_titleY(0),
      m_headerY(0),
      m_backing(),
      m_text(),
      m_thumb(COLOR_THUMB),
      m_track(COLOR_TRACK),
      m_button(Button::QUAD_GROUP, Button::QUAD_MESH, TEXT_BACK),
      m_viewHeight(0),
      m_textHeight(0),
      m_prevScroll(0),
      m_scroll(0),
      m_pointerId(-1),
      m_pointerY(0),
      m_dragScroll(0),
      m_velocity(0.0f) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int width = screen->getLocalWidth();
    int height = screen->getLocalHeight();
    int baseline17 = text->getBaseline(FONT_HEADER);
    m_titleY = baseline17 + 22;
    m_headerY = baseline17 + 16;
    int baseline16 = text->getBaseline(FONT_TITLE);
    if (titleId != TEXT_NONE) {
        m_title = text->getString(titleId)->toUpperCase();
    }
    if (headerId != TEXT_NONE) {
        m_header = text->getString(headerId)->toUpperCase();
    }
    int titles = m_titleY + baseline16;
    m_button.setTopLeft(width - 30 - m_button.getWidth(), height - 30 - m_button.getHeight());
    int content = height - 46 - (titles + m_button.getHeight());
    m_backing.setBounds(80, titles + 16, width - 160, content - 16);
    m_text.wrapString(textId, FONT_BODY, width - 206);
    m_viewHeight = content - 40;
    m_textHeight = m_text.getWrappedTextHeight();
    if (m_viewHeight < m_textHeight) {
        int thumbH = (int)((float)m_viewHeight * ((float)m_viewHeight / (float)m_textHeight));
        int barX = width - 106;
        int barY = titles + 28;
        m_thumb.setBounds(barX, barY, 16, thumbH);
        m_track.setBounds(barX, barY, 16, m_viewHeight);
    }
}

void ScrollingText::moveScroller(int delta) {
    m_dragScroll += delta;
    int limit = m_textHeight - m_viewHeight;
    m_scroll = midp::Math::max(0, midp::Math::min(limit, m_dragScroll));
}

void ScrollingText::update(int dt) {
    float seconds = (float)dt * 0.001f;
    float speed = (float)(m_scroll - m_prevScroll) / seconds;
    if (m_pointerId == -1) {
        float diff = speed - m_velocity;
        float step = diff * seconds * -5.0f + diff;
        if ((diff < 0.0f) != (step < 0.0f)) {
            step = 0.0f;
        }
        m_velocity = speed - step;
        moveScroller((int)(m_velocity * seconds));
    } else {
        m_velocity = m_velocity * 0.25f + speed * 0.75f;
    }
    m_prevScroll = m_scroll;
}

int ScrollingText::pointerReleased(int pressX, int pressY, int x, int y, int id, GameScreen*) {
    if (m_pointerId == id) {
        m_pointerId = -1;
    }
    return m_button.pointerReleased(pressX, pressY, x, y, m_screen) ? 1 : 0;
}

void ScrollingText::pointerDragged(int pressX, int pressY, int x, int y, int id) {
    if (m_pointerId == id) {
        moveScroller(m_pointerY - y);
        m_pointerY = y;
    }
    m_button.pointerDragged(pressX, pressY, x, y);
}

void ScrollingText::pointerPressed(int x, int y, int id) {
    if (m_viewHeight < m_textHeight && m_backing.isPointWithin(x, y)) {
        m_pointerId = id;
        m_pointerY = y;
        m_dragScroll = m_scroll;
    }
    m_button.pointerPressed(x, y);
}

void ScrollingText::render2D(midp::Graphics* g) {
    int width = m_screen->getLocalWidth();
    int height = m_screen->getLocalHeight();
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    int labelX = m_backing.getLeft() + 6;
    text->drawString(g, m_header, FONT_HEADER, labelX, m_headerY, ANCHOR_LEFT_BASELINE);
    text->drawString(g, m_title, FONT_TITLE, labelX, m_titleY, ANCHOR_LEFT_TOP);
    int left = m_backing.getLeft();
    int top = m_backing.getTop();
    int clipY = top + 12;
    g->setClip(0, clipY, width, m_backing.getHeight() - 24);
    m_text.draw(g, left + 10, clipY - m_scroll, ANCHOR_LEFT_TOP);
    g->setClip(0, 0, width, height);
    m_button.render2D(g);
}

void ScrollingText::render3D(m3g::Graphics3D* g3d) {
    m_backing.render3D(g3d);
    m_button.render3D(g3d);
    float ratio = (float)m_scroll / (float)m_textHeight;
    m_thumb.setTop(m_track.getTop() + (int)((float)m_track.getHeight() * ratio));
    m_track.render3D(g3d);
    m_thumb.render3D(g3d);
}

ScrollingText::~ScrollingText() {}

}
