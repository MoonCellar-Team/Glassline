#include "game/AchievementNotification.h"

#include <algorithm>

#include "game/Achievement.h"
#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int STATE_LOAD = 0;
static const int STATE_IN = 1;
static const int STATE_HOLD = 2;
static const int STATE_OUT = 3;
static const int STATE_PAUSE = 4;

static const int HOLD_MS = 2000;
static const int PAUSE_MS = 500;
static const int FADE_MS = 1000;
static const float SLIDE_SPEED = 40.0f;
static const float MS_TO_SEC = 0.001f;

static const int INITIAL_WIDTH = 200;
static const int INITIAL_HEIGHT = 0x28;
static const int WIDTH_PAD = 0x55;
static const int SCREEN_MARGIN = 0x14;
static const int BADGE_INSET = 0xf;
static const int TEXT_PAD = 0x37;
static const int NAME_Y = 2;
static const int DESC_FROM_BOTTOM = 4;

static const int FONT_NAME = 0x14;
static const int FONT_DESCRIPTION = 0x15;
static const int ANCHOR_NAME = 10;
static const int ANCHOR_DESCRIPTION = 0x22;

static const int GROUP_BADGE = 0x39;
static const int MESH_INCOMPLETE = 0x93;
static const int MESH_COMPLETE = 0x94;

namespace UI {

void AchievementNotification::update(int dt) {
    if (m_state < 5) {
        switch (m_state) {
        case STATE_LOAD: {
            Achievement* achievement = m_queue.front();
            m_title = achievement->getNameStringBuffer()->toString();
            m_description = achievement->getDescriptionStringBuffer()->toString();
            TextManager* text = AppEngine::getCanvas()->getTextManager();
            int nameWidth = text->getStringWidth(m_title, FONT_NAME);
            int descWidth = text->getStringWidth(m_description, FONT_DESCRIPTION);
            setWidth(std::max(nameWidth, descWidth) + WIDTH_PAD);
            m_alpha = 0.0f;
            m_state = STATE_IN;
            m_timer = 0;
            break;
        }
        case STATE_IN:
            m_slide += static_cast<float>(dt) * MS_TO_SEC * SLIDE_SPEED;
            if (m_slide >= static_cast<float>(m_height)) {
                m_slide = static_cast<float>(m_height);
                m_state = STATE_HOLD;
                m_timer = HOLD_MS;
            }
            break;
        case STATE_HOLD:
            m_timer -= dt;
            m_alpha = static_cast<float>(m_timer - FADE_MS) * -MS_TO_SEC + 1.0f;
            m_alpha = std::min(m_alpha, 1.0f);
            if (m_timer < 1) {
                m_state = STATE_OUT;
            }
            break;
        case STATE_OUT:
            m_slide += static_cast<float>(dt) * MS_TO_SEC * -SLIDE_SPEED;
            if (m_slide <= 0.0f) {
                m_queue.pop();
                m_state = STATE_PAUSE;
                m_timer = PAUSE_MS;
                m_slide = 0.0f;
            }
            break;
        case STATE_PAUSE:
            if ((m_timer < 1 || (m_timer -= dt) < 1) && m_queue.size() != 0) {
                m_state = STATE_LOAD;
                m_timer = 0;
            }
            break;
        }
    }
    float y = static_cast<float>(AppEngine::getCanvas()->getHeight()) - m_slide;
    int x = (AppEngine::getCanvas()->getWidth() - SCREEN_MARGIN) - m_width;
    setPosition(x, static_cast<int>(y));
}

void AchievementNotification::render(midp::Graphics* g, int originY, int originX) {
    if ((m_state | 4) == 4) {
        return;
    }
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    m_quadManager->setGroupVisible(GROUP_BADGE, true);
    float badgeX = static_cast<float>(originX + m_x + BADGE_INSET);
    float badgeY = static_cast<float>(m_y + originY + (m_height >> 1));
    m_quadManager->setGroupPosition(GROUP_BADGE, badgeX, badgeY);
    m_quadManager->setMeshVisible(MESH_INCOMPLETE, true);
    m_quadManager->setMeshVisible(MESH_COMPLETE, true);
    m_quadManager->setMeshAlpha(MESH_COMPLETE, m_alpha);
    BorderedElement::render(g, originY, originX);
    m_quadManager->setGroupVisible(GROUP_BADGE, false);

    int textX = ((m_width - TEXT_PAD) >> 1) + TEXT_PAD;
    text->drawString(g, m_title, FONT_NAME, m_x + originX + textX, originY + m_y + NAME_Y, ANCHOR_NAME);
    text->drawString(g, m_description, FONT_DESCRIPTION, textX + originX + m_x,
                     originY + m_y + m_height - DESC_FROM_BOTTOM, ANCHOR_DESCRIPTION);
}

AchievementNotification::AchievementNotification()
    : BorderedElement(0, 0, INITIAL_WIDTH, INITIAL_HEIGHT),
      m_state(STATE_PAUSE),
      m_timer(0),
      m_slide(0.0f),
      m_alpha(0.0f),
      m_title(NULL),
      m_description(NULL) {}

void AchievementNotification::addAchievement(Achievement* achievement) {
    m_queue.push(achievement);
}

AchievementNotification::~AchievementNotification() {}

}
