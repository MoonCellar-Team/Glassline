#include "game/MenuPause.h"

#include <stddef.h>

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/MenuMainSubMenu.h"
#include "game/QuadManager.h"
#include "game/TextManager.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "runtime/midp/String.h"
#ifdef MEDECOMP_PORT
extern "C" midp::String* medecomp_player_level_title(void);
#endif

static const int TEXT_GAME_PAUSED = 0x828;

#ifdef MEDECOMP_PORT
static void usePlayerLevelName(midp::ReferenceCountedPointer<midp::String>& levelName) {
    midp::ReferenceCountedPointer<midp::String> own(medecomp_player_level_title());
    if (own) {
        levelName = own->toUpperCase();
    }
}
#endif

void MenuPause::render3D(midp::Graphics* g, m3g::Graphics3D* g3d, int width, int height) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(m_quadGroup, true);
    Menu::render3D(g, g3d, width, height);
    quads.setGroupVisible(m_quadGroup, false);
}

void MenuPause::render2D(midp::Graphics* g, int width, int height) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    Menu::render2D(g, width, height);
    int lineHeight = text->getLineHeight(40);
    text->drawString(g, m_levelName, 17, 55, 43, 9);
    text->drawString(g, m_title, 16, 55, lineHeight + 43, 9);
}

MenuPause::MenuPause() : m_levelName(NULL), m_title(NULL), m_quadGroup(49) {}

void MenuPause::createSplit(bool first, int height) {
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    m_labelWidth = 120;
    m_subMenus = midp::array<MenuMainSubMenu>(4);
    if (first) {
        m_subMenus[0].init(height, 75, 166, 79, 20);
        m_subMenus[1].init(height, 76, 167, 80, 21);
        m_subMenus[2].init(height, 77, 168, 81, 22);
        m_subMenus[3].init(height, 78, 169, 82, 23);
    } else {
        m_subMenus[0].init(height, 75, 166, 79, 24);
        m_subMenus[1].init(height, 76, 167, 80, 25);
        m_subMenus[2].init(height, 77, 168, 81, 26);
        m_subMenus[3].init(height, 78, 169, 82, 27);
    }
    m_quadGroup = 74;
    m_subMenus[0].setScollingUVs(70, 71, 72, 73);
    m_subMenus[1].setScollingUVs(74, 75, 76, 77);
    m_subMenus[2].setScollingUVs(78, 79, 80, 81);
    m_subMenus[3].setScollingUVs(82, 83, 84, 85);
    m_subMenus[0].create(0, ITEM_RESUME);
    m_subMenus[1].create(0, ITEM_RESTART);
    m_subMenus[2].create(0, ITEM_QUIT);
    m_subMenus[3].create(4, ITEM_OPTIONS);
    m_subMenus[3].append(ITEM_SOUND);
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOSMediaPickerSupported();
    m_subMenus[3].append(ITEM_HELP);
    m_levelName = text->getString(level->getName())->toUpperCase();
#ifdef MEDECOMP_PORT
    usePlayerLevelName(m_levelName);
#endif
    m_title = text->getString(TEXT_GAME_PAUSED)->toUpperCase();
    canvas->getQuadManager().setMeshVisible(94, false);
}

void MenuPause::createSingle() {
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    int height = canvas->getHeight();
    m_labelWidth = 160;
    m_subMenus = midp::array<MenuMainSubMenu>(4);
    m_subMenus[0].init(height, 59, 90, 63, 12);
    m_subMenus[0].setScollingUVs(70, 71, 72, 73);
    m_subMenus[0].create(0, ITEM_RESUME);
    m_subMenus[1].init(height, 60, 91, 64, 13);
    m_subMenus[1].setScollingUVs(74, 75, 76, 77);
    m_subMenus[1].create(0, ITEM_RESTART);
    m_subMenus[2].init(height, 61, 92, 65, 14);
    m_subMenus[2].setScollingUVs(78, 79, 80, 81);
    m_subMenus[2].create(0, ITEM_QUIT);
    m_subMenus[3].init(height, 62, 93, 66, 15);
    m_subMenus[3].setScollingUVs(82, 83, 84, 85);
    m_subMenus[3].create(4, ITEM_OPTIONS);
    m_subMenus[3].append(ITEM_SOUND);
    m_subMenus[3].append(ITEM_SETTINGS);
    static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOSMediaPickerSupported();
    m_subMenus[3].append(ITEM_HELP);
    m_levelName = text->getString(level->getName())->toUpperCase();
#ifdef MEDECOMP_PORT
    usePlayerLevelName(m_levelName);
#endif
    m_title = text->getString(TEXT_GAME_PAUSED)->toUpperCase();
    canvas->getQuadManager().setMeshVisible(94, false);
}

MenuPause::~MenuPause() {}
