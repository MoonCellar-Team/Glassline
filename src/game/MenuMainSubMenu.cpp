#include "game/MenuMainSubMenu.h"

#include <stddef.h>

#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/GameScreen.h"
#include "game/QuadManager.h"
#include "game/QuadTexture.h"
#include "game/TextManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/WindowStore.h"
#include "game/WrappedString.h"
#include "m3g/Texture2D.h"
#include "runtime/midp/Graphics.h"

static const int FIRST_ROW_Y = 180;
static const int ROW_SPACING = 90;
static const int ROW_HALF_HEIGHT = 47;
static const int BAR_HEIGHT = 95;
static const int BAR_ANIM_TIME = 500;

static const int SWAY_INTERVAL = 2250;
static const float SWAY_AMPLITUDE = 8.0f;
static const float SWAY_FILTER_PARAM = 3000.0f;
static const float SCROLL_SPEED = 0.0002f;

static const int FONT_ITEM = 8;
static const int FONT_ITEM_SELECTED = 7;
static const int FONT_TITLE = 8;
static const int FONT_TITLE_ACTIVE = 13;

static const int SCOPE_MENU_LIST = 0x40;

bool MenuMainSubMenu::isSelected() {
    if (!m_selected) {
        return false;
    }
    return m_barAnim.isFinished();
}

void MenuMainSubMenu::init(int height, int ribbonGroup, int buttonMesh, int activeGroup, int anim) {
    m_screenHeight = height;
    m_selected = false;
    m_ribbonGroup = ribbonGroup;
    m_buttonMesh = buttonMesh;
    m_activeGroup = activeGroup;
    m_anim = anim;
}

bool MenuMainSubMenu::isAnimating() {
    return m_animState == ANIM_CLOSING || m_animState == ANIM_OPENING;
}

void MenuMainSubMenu::pointerReleased(int x, int y, GameScreen* screen) {
    if (m_animState == ANIM_ACTIVE) {
        AppEngine* canvas = AppEngine::getCanvas();
        QuadManager& quads = canvas->getQuadManager();
        int count = getLength();
        int top = m_screenHeight - FIRST_ROW_Y - ROW_HALF_HEIGHT;
        int bottom = m_screenHeight - FIRST_ROW_Y + ROW_HALF_HEIGHT;
        for (int i = 0; i != count; i++, top -= ROW_SPACING, bottom -= ROW_SPACING) {
            if (top <= y && y <= bottom) {
                if (m_highlightIndex == i) {
                    setSelectedIndex(m_highlightIndex);
                    m_selected = true;
                    int effectX = quads.getMeshX(m_buttonMesh);
                    int effectY = top + ROW_HALF_HEIGHT;
                    if (screen != NULL) {
                        screen->calcGlobalCoordinates(effectX, effectY, effectX, effectY);
                    }
                    canvas->getWindowStore().getButtonEffect()->play(effectX, effectY);
                }
                break;
            }
        }
    }
    m_highlightIndex = -1;
}

void MenuMainSubMenu::animateToSelection(int index, bool immediate) {
    if (immediate) {
        m_barAnim.stop();
        m_queuedIndex = -1;
        setSelectedIndex(index);
        return;
    }
    if (!m_barAnim.isFinished()) {
        m_queuedIndex = (m_highlightIndex == index) ? -1 : index;
    } else if (m_highlightIndex != index) {
        int base = m_screenHeight - FIRST_ROW_Y;
        int from = base - ROW_SPACING * getSelectBarIndex();
        m_barAnim.start((float)from, (float)(base - ROW_SPACING * index), BAR_ANIM_TIME,
                        InterpolationTimed::EASE_OUT_CUBIC);
        m_highlightIndex = index;
    }
}

void MenuMainSubMenu::pointerOff(int x, int y) {
    animateToSelection(getSelectedIndex(), false);
}

void MenuMainSubMenu::pointerOn(int x, int y) {
    int count = getLength();
    if (count == 0) {
        m_selected = true;
        return;
    }
    if (m_animState != ANIM_ACTIVE) {
        return;
    }
    int top = m_screenHeight - FIRST_ROW_Y - ROW_HALF_HEIGHT;
    int bottom = m_screenHeight - FIRST_ROW_Y + ROW_HALF_HEIGHT;
    for (int i = 0; i != count; i++, top -= ROW_SPACING, bottom -= ROW_SPACING) {
        if (top <= y && y <= bottom) {
            animateToSelection(i, false);
            return;
        }
    }
}

void MenuMainSubMenu::refreshAnim(QuadManager& quads) {
    QuadManager& canvasQuads = AppEngine::getCanvas()->getQuadManager();
    float sway = m_sway.getFilteredValue();
    canvasQuads.setGroupPosition(m_ribbonGroup, sway, 0.0f);
    canvasQuads.setGroupPosition(m_activeGroup, sway, 0.0f);
    m_stripTex0->setTranslation(0.0f, m_scroll0, 0.0f);
    m_stripTex1->setTranslation(0.0f, m_scroll1, 0.0f);
    m_stripTex2->setTranslation(0.0f, m_scroll0, 0.0f);
    m_stripTex3->setTranslation(0.0f, m_scroll1, 0.0f);
    quads.getAnimPlayer(m_anim)->refreshMeshes();
    quads.setMeshVisible(m_buttonMesh, m_animState == ANIM_IDLE);
}

void MenuMainSubMenu::renderActiveRibbonLabels(midp::Graphics* g, int labelWidth, int width, int height) {
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    int x = canvas->getQuadManager().getMeshX(m_buttonMesh) - (labelWidth >> 1);
    int count = getLength();
    int selected = getSelectBarIndex();
    int y = height - FIRST_ROW_Y;
    for (int i = 0; i != count; i++, y -= ROW_SPACING) {
        int font = (i == selected) ? FONT_ITEM_SELECTED : FONT_ITEM;
        if (isStringsWrapped()) {
            getMenuItemWrappedString(i).drawWithFont(g, font, x, y, 17);
        } else {
            text->drawString(g, getItem(i), font, x, y, 17);
        }
    }
}

void MenuMainSubMenu::render2D(midp::Graphics* g, int labelWidth, int width, int height) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    TextManager* text = canvas->getTextManager();
    if (m_animState == ANIM_ACTIVE) {
        renderActiveRibbonLabels(g, labelWidth, width, height);
    }
    int font = (m_animState == ANIM_ACTIVE) ? FONT_TITLE_ACTIVE : FONT_TITLE;
    midp::String* title = getThisTitleAllCapsString();
    int x = quads.getMeshX(m_buttonMesh) - 3;
    int y = quads.getMeshY(m_buttonMesh) - 1;
    text->drawString(g, title, font, x, y, 18);
}

void MenuMainSubMenu::render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height) {
    if (m_animState != ANIM_ACTIVE) {
        return;
    }
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int barY;
    if (!m_barAnim.isFinished()) {
        barY = (int)m_barAnim.getCurrentValue();
    } else {
        barY = height - FIRST_ROW_Y - ROW_SPACING * getSelectBarIndex();
    }
    g->setClip(0, barY - ROW_HALF_HEIGHT, width, BAR_HEIGHT);
    quads.setGroupVisible(m_activeGroup, true);
    quads.render(g3d, SCOPE_MENU_LIST);
    quads.setGroupVisible(m_activeGroup, false);
    g->setClip(0, 0, width, height);
}

void MenuMainSubMenu::setScollingUVs(int tex0, int tex1, int tex2, int tex3) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    m_stripTex0 = quads.getTexture(tex0)->m_texture;
    m_stripTex1 = quads.getTexture(tex1)->m_texture;
    m_stripTex2 = quads.getTexture(tex2)->m_texture;
    m_stripTex3 = quads.getTexture(tex3)->m_texture;
    m_scroll0 = canvas->randPercentile();
    m_scroll1 = canvas->randPercentile();
    m_stripTex0->setTranslation(0.0f, m_scroll0, 0.0f);
    m_stripTex1->setTranslation(0.0f, m_scroll1, 0.0f);
    m_stripTex2->setTranslation(0.0f, m_scroll0, 0.0f);
    m_stripTex3->setTranslation(0.0f, m_scroll1, 0.0f);
}

void MenuMainSubMenu::stateTransition(AnimState state) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    m_animState = state;
    switch (state) {
    case ANIM_IDLE:
        quads.setAnimFrame(m_anim, 0);
        break;
    case ANIM_OPENING:
        quads.playAnim(m_anim, 2);
        m_sway.setTargetValue(0.0f);
        m_selected = false;
        break;
    case ANIM_ACTIVE:
        quads.setAnimFrame(m_anim, 1);
        break;
    case ANIM_CLOSING:
        quads.playAnimReverse(m_anim, 2);
        break;
    }
}

void MenuMainSubMenu::setOpen(int index) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setAnimFrame(m_anim, 1);
    m_sway.setTargetValue(0.0f);
    m_selected = false;
    stateTransition(ANIM_ACTIVE);
    animateToSelection(index, true);
}

void MenuMainSubMenu::transitionToActive() {
    if (m_animState == ANIM_IDLE) {
        stateTransition(ANIM_OPENING);
    }
#ifdef MEDECOMP_PORT
    else if (m_animState == ANIM_CLOSING) {
        AppEngine::getCanvas()->getQuadManager().playAnim(m_anim, 0);
        m_sway.setTargetValue(0.0f);
        m_selected = false;
        m_animState = ANIM_OPENING;
    }
#endif
}

void MenuMainSubMenu::transitionToIdle() {
    if (m_animState == ANIM_ACTIVE) {
        stateTransition(ANIM_CLOSING);
    }
#ifdef MEDECOMP_PORT
    else if (m_animState == ANIM_OPENING) {
        AppEngine::getCanvas()->getQuadManager().playAnimReverse(m_anim, 0);
        m_animState = ANIM_CLOSING;
    }
#endif
}

void MenuMainSubMenu::update(int dt) {
    float scroll = dt * SCROLL_SPEED;
    m_scroll0 += scroll;
    while (m_scroll0 >= 1.0f) {
        m_scroll0 -= 1.0f;
    }
    m_scroll1 += scroll;
    while (m_scroll1 >= 1.0f) {
        m_scroll1 -= 1.0f;
    }
    m_sway.update(dt);
    if (!m_barAnim.isFinished()) {
        m_barAnim.update(dt);
        if (m_barAnim.isFinished() && m_queuedIndex != -1) {
            int index = m_queuedIndex;
            m_queuedIndex = -1;
            animateToSelection(index, false);
        }
    }
    switch (m_animState) {
    case ANIM_IDLE:
        m_swayTimer -= dt;
        if (m_swayTimer <= 0) {
            m_swayTimer = SWAY_INTERVAL;
            m_swayDir = -m_swayDir;
            m_sway.setTargetValue(AppEngine::getCanvas()->randPercentile() * m_swayDir * SWAY_AMPLITUDE);
        }
        break;
    case ANIM_OPENING: {
        QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
        quads.updateAnim(m_anim, dt);
        if (!quads.isAnimating(m_anim)) {
            stateTransition(ANIM_ACTIVE);
        }
        break;
    }
    case ANIM_CLOSING: {
        QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
        quads.updateAnim(m_anim, dt);
        if (!quads.isAnimating(m_anim)) {
            stateTransition(ANIM_IDLE);
        }
        break;
    }
    }
}

MenuMainSubMenu::~MenuMainSubMenu() {}

MenuMainSubMenu::MenuMainSubMenu(const MenuMainSubMenu& other)
    : MenuStringChoice(other),
      m_screenHeight(other.m_screenHeight),
      m_selected(other.m_selected),
      m_animState(other.m_animState),
      m_unk64(other.m_unk64),
      m_ribbonGroup(other.m_ribbonGroup),
      m_buttonMesh(other.m_buttonMesh),
      m_activeGroup(other.m_buttonMesh),
      m_anim(other.m_anim),
      m_sway(SignalFilter::ONE_POLE_LP, SWAY_FILTER_PARAM, 0.0f),
      m_swayTimer(other.m_swayTimer),
      m_swayDir(other.m_swayDir),
      m_scroll0(other.m_scroll0),
      m_scroll1(other.m_scroll1),
      m_stripTex0(other.m_stripTex0),
      m_stripTex1(other.m_stripTex1),
      m_stripTex2(other.m_stripTex2),
      m_stripTex3(other.m_stripTex3),
      m_highlightIndex(-1),
      m_queuedIndex(-1) {}

MenuMainSubMenu::MenuMainSubMenu()
    : m_screenHeight(0),
      m_selected(false),
      m_animState(ANIM_IDLE),
      m_unk64(0),
      m_ribbonGroup(-1),
      m_buttonMesh(-1),
      m_activeGroup(-1),
      m_anim(-1),
      m_sway(SignalFilter::SMOOTHSTEP_FIR, SWAY_FILTER_PARAM, 0.0f),
      m_swayTimer(3000),
      m_swayDir(1),
      m_scroll0(0.0f),
      m_scroll1(0.0f),
      m_stripTex0(NULL),
      m_stripTex1(NULL),
      m_stripTex2(NULL),
      m_stripTex3(NULL),
      m_highlightIndex(-1),
      m_queuedIndex(-1) {
    AppEngine* canvas = AppEngine::getCanvas();
    m_screenHeight = canvas->getHeight();
    m_swayTimer = canvas->rand(1500);
    m_swayDir = (canvas->rand(2) >> 1) - 1;
}

int MenuMainSubMenu::getButtonMeshId() {
    return m_buttonMesh;
}

bool MenuMainSubMenu::isActive() const {
    return m_animState == ANIM_ACTIVE;
}

int MenuMainSubMenu::getSelectBarIndex() {
    if (m_highlightIndex == -1) {
        return getSelectedIndex();
    }
    return m_highlightIndex;
}
