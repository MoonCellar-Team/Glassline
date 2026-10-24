#include "game/GameScreen.h"

#include <stddef.h>

#include "ea_online/SpywareManager.h"
#include "game/AnimPlayerQuad.h"
#include "game/AppEngine.h"
#include "game/GameCamera.h"
#include "game/GameCommon.h"
#include "game/GameObjectCheckpoint.h"
#include "game/GameObjectPlayer.h"
#include "game/GameUI/Confirmation.h"
#include "game/GameUI/EndOfLevelResults.h"
#include "game/GameUI/ScrollingText.h"
#include "game/GameUI/SoundSettings.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/MEdgeMap.h"
#include "game/MathTrig.h"
#include "game/MenuMainSubMenu.h"
#include "game/QuadManager.h"
#include "game/ReflectionMapShader.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "m3g/Camera.h"
#include "m3g/Graphics3D.h"
#include "m3g/World.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
#endif
#ifdef MEDECOMP_SPLIT_STACKED
#include "game/QuadGroup.h"
#include "game/QuadMesh.h"
#endif

#ifdef MEDECOMP_PORT
extern "C" void medecomp_post_world(float fovy, float aspect, float nearPlane, float farPlane);
extern "C" int medecomp_player_level_playing(void);
extern "C" int medecomp_player_level_best(void);
#endif

static const int MESH_COLLECTABLES = 70;
static const int MESH_COLLECTABLES_RIVAL = 71;
#ifdef MEDECOMP_PORT
static const int TEXT_THEIR_BAGS = 0x946;
#endif
static const int MESH_LOCATOR = 72;
static const int MESH_RACE_TIMER = 77;

enum MenuItem {
    ITEM_HELP = 0x803,
    ITEM_GAME_OPTIONS = 0x822,
    ITEM_MEDIA_PICKER = 0x826,
    ITEM_RESUME = 0x829,
    ITEM_RESTART = 0x82a,
    ITEM_QUIT = 0x82b,
    ITEM_SOUND_SETTINGS = 0x8d7,
};

static const int TAP_MAX_MS = 750;
static const int SWIPE_MIN_DIST = 30;
static const int SWIPE_MAX_SIDE = 15;

static int iabs(int v) {
    return v < 0 ? -v : v;
}

bool GameScreen::isInRaceStart() {
    return m_state >= SCREEN_RACE_START_3 && m_state <= SCREEN_RACE_START_1;
}

MathFrustum* GameScreen::getCameraFrustum() {
    return &m_frustum;
}

GameObjectCheckpoint* GameScreen::getCheckpointObject() {
    return m_checkpoint;
}

GameCamera* GameScreen::getGameCamera() {
    return m_camera;
}

GameObjectPlayer* GameScreen::getPlayerObject() {
    return m_player;
}

void GameScreen::setPlayerObject(GameObjectPlayer* player) {
    m_player = player;
}

int GameScreen::getState() {
    return m_state;
}

void GameScreen::startCameraShake(float intensity, int duration) {
    m_camera->startShake(intensity, duration);
}

int GameScreen::getEffectScope() {
    return m_effectScope;
}

#ifdef MEDECOMP_SPLIT_STACKED
static void stackSplitQuads(int width, int height) {
    static bool s_done = false;
    if (s_done) {
        return;
    }
    s_done = true;
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    const int ipadWidth = AppEngine::getCanvas()->getHeight();
    const int ipadHeight = (AppEngine::getCanvas()->getWidth() - 10) >> 1;
    auto shift = [&quads](int mesh, float dx, float dy) {
        QuadMesh* m = quads.getMesh(mesh);
        m->m_x += dx;
        m->m_y += dy;
        quads.updateMesh(m);
    };
    for (int i = 0; i < 4; i++) {
        const int button = 166 + i;
        float dx = (float)((2 * i + 1) * width / 8) - quads.getMeshX(button);
        for (int group : {75 + i, 79 + i}) {
            const QuadGroup& g = quads.m_groups[group];
            for (int mesh = g.m_firstMesh; mesh < g.m_firstMesh + g.m_numMeshes; mesh++) {
                shift(mesh, dx, 0.0f);
            }
        }
        shift(button, dx, (float)(height - ipadHeight));
    }
    for (int mesh : {MESH_COLLECTABLES_RIVAL, 73, 74}) {
        shift(mesh, (float)(width - ipadWidth), 0.0f);
    }
}
#endif

GameScreen::GameScreen(SceneGame* scene, ScreenType type, m3g::World* world)
    : m_scene(scene),
      m_world(world),
      m_type(type),
      m_width(0),
      m_height(0),
      m_state(SCREEN_GAME),
      m_resumeState(SCREEN_GAME),
      m_cameraPos(3),
      m_inputFlags(0),
      m_effectScope(0),
      m_fadeMesh(-1),
      m_fadeAnim(-1),
      m_damageMesh(-1),
      m_painAnim(-1),
      m_flashAnim(-1),
      m_collectMesh(-1),
      m_collectAnim(-1),
      m_showInverseCount(false),
      m_painSoundHandle(-1),
      m_collectHudTimer(0),
      m_checkpoint(NULL),
      m_dimRect((int)0x99ffffff),
      m_raceStartTimer(0) {
    m_m3gCamera = new m3g::Camera();
    world->addChild(m_m3gCamera);
    m_camera = new GameCamera(m_type != TYPE_SINGLE);
    m_camera->setM3GCamera(m_m3gCamera);
    m_camera->loadM3GAnims();

    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    int canvasWidth = canvas->getWidth();
    int canvasHeight = canvas->getHeight();
    int painMesh = -1;
    int flashMesh = -1;
    switch (type) {
    case TYPE_SINGLE:
        m_effectScope = 0x10;
        m_fadeMesh = 81;
        m_fadeAnim = 6;
        m_damageMesh = 78;
        m_painAnim = 4;
        m_flashAnim = 5;
        m_width = canvasWidth;
        m_height = canvasHeight;
        painMesh = 79;
        flashMesh = 80;
#ifdef MEDECOMP_PORT
        if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
            m_collectMesh = 73;
            m_collectAnim = 2;
        }
#endif
        break;
    case TYPE_SPLIT_FIRST:
        m_effectScope = 0x10;
        m_fadeMesh = 81;
        m_fadeAnim = 6;
        m_damageMesh = 78;
        m_painAnim = 4;
        m_flashAnim = 5;
        m_collectMesh = 73;
        m_collectAnim = 2;
        m_width = canvasHeight;
        m_height = (canvasWidth - 10) >> 1;
        painMesh = 79;
        flashMesh = 80;
        break;
    case TYPE_SPLIT_SECOND:
        m_effectScope = 0x20;
        m_fadeMesh = 85;
        m_fadeAnim = 9;
        m_damageMesh = 82;
        m_painAnim = 7;
        m_flashAnim = 8;
        m_collectMesh = 74;
        m_collectAnim = 3;
        m_width = canvasHeight;
        m_height = (canvasWidth - 10) >> 1;
        painMesh = 83;
        flashMesh = 84;
        break;
    }
#ifdef MEDECOMP_SPLIT_STACKED
    if (type != TYPE_SINGLE) {
        m_width = canvasWidth;
        m_height = (canvasHeight - 10) >> 1;
        stackSplitQuads(m_width, m_height);
    }
#endif
    m_dimRect.setBounds(0, 0, m_width, m_height);
    float effectX = 0.0f;
    float effectWidth = m_width;
#ifdef MEDECOMP_PORT
    bool wideEffects = m_type == TYPE_SINGLE;
#ifdef MEDECOMP_SPLIT_STACKED
    wideEffects = true;
#endif
    if (wideEffects) {
        effectX = (float)-medecomp_wide_extra();
        effectWidth = m_width + 2.0f * medecomp_wide_extra();
    }
#endif
    quads.setMeshBounds(m_fadeMesh, effectX, 0.0f, effectWidth, m_height, 9);
    quads.setMeshBounds(m_damageMesh, effectX, 0.0f, effectWidth, m_height, 9);
    quads.setMeshBounds(painMesh, effectX, 0.0f, effectWidth, m_height, 9);
    quads.setMeshBounds(flashMesh, effectX, 0.0f, effectWidth, m_height, 9);
    quads.setAnimFrame(m_fadeAnim, 0);
    quads.setAnimFrame(m_painAnim, 1);
    quads.setAnimFrame(m_flashAnim, 1);
}

void GameScreen::calculateVectorToScreenSidePositive(int& x, int& y) {
    int halfWidth = m_width >> 1;
    int halfHeight = m_height >> 1;
    float fx = (float)x / halfWidth;
    float fy = (float)y / halfHeight;
    if (fy >= fx) {
        x = (int)(halfWidth * (fx / fy));
        y = halfHeight;
    } else {
        x = halfWidth;
        y = (int)(halfHeight * (fy / fx));
    }
}

void GameScreen::calculateVectorToScreenSide(int& x, int& y) {
    if (x == 0) {
        y = y < 0 ? -(m_height >> 1) : m_height >> 1;
        return;
    }
    if (y == 0) {
        x = x < 0 ? -(m_width >> 1) : m_width >> 1;
        return;
    }
    bool negX = x < 0;
    bool negY = y < 0;
    if (negX) {
        x = -x;
    }
    if (negY) {
        y = -y;
    }
    calculateVectorToScreenSidePositive(x, y);
    if (negX) {
        x = -x;
    }
    if (negY) {
        y = -y;
    }
}

GameScreenPointer* GameScreen::getPointer(int id) {
    if (m_pointer1 == id) {
        return &m_pointer1;
    }
    if (m_pointer2 == id) {
        return &m_pointer2;
    }
    return NULL;
}

void GameScreen::pointerReleasedGame(GameScreenPointer* pointer) {
    if (pointer == &m_pointer1 && pointer->m_holdTime < TAP_MAX_MS && !pointer->m_swiped) {
        m_inputFlags |= INPUT_TAP;
    }
}

void GameScreen::pointerReleasedMultiplayerComplete(GameScreenPointer* pointer) {
    if (pointer != &m_pointer1) {
        return;
    }
    if (m_results->pointerReleased(pointer->m_pressX, pointer->m_pressY, pointer->m_x, pointer->m_y, this) == 1) {
        m_scene->setExitState(SceneGame::STATE_EXIT_LEVEL);
        m_scene->stateTransition(SceneGame::STATE_EXIT_FADE);
    }
}

void GameScreen::initMultiplayerComplete() {
    m_results = new GameUI::EndOfLevelResults(m_scene, this);
}

void GameScreen::renderMenuMain2D(midp::Graphics* g) {
    m_menu.render2D(g, m_width, m_height);
    if (AppEngine::getLevelData()->getGameMode() != LevelData::GAME_MODE_STORY) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    if (!quads.getMeshVisible(MESH_COLLECTABLES)) {
        return;
    }
    int y = quads.getMeshBottom(MESH_COLLECTABLES) + 10;
    int count = m_player->getNumCollectablesCollected();
    int total = m_scene->getMap().getNumCollectablesTotal();
    canvas->drawOfStatString(g, 8, 0x800, count, total, m_width - 66, y, 10);
}

void GameScreen::renderMenuMain3D(midp::Graphics* g, m3g::Graphics3D* g3d) {
    m_menu.render3D(g, g3d, m_width, m_height);
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    if (AppEngine::getLevelData()->getGameMode() != LevelData::GAME_MODE_STORY) {
        return;
    }
    MenuMainSubMenu* subMenu = m_menu.getSubMenu(3);
    if (subMenu->isActive() || subMenu->isAnimating()) {
        quads.setMeshVisible(MESH_COLLECTABLES, false);
    } else {
        quads.setMeshVisible(MESH_COLLECTABLES, true);
        quads.render(g3d, 0x100);
    }
}

void GameScreen::renderHud3D(m3g::Graphics3D* g3d) {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int mode = AppEngine::getLevelData()->getGameMode();
#ifdef MEDECOMP_PORT
    if (mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED && m_type == TYPE_SINGLE) {
        bool results = m_scene->getState() == SceneGame::STATE_LEVEL_COMPLETE_MULTIPLAYER;
        quads.setMeshVisible(MESH_COLLECTABLES, !results);
        quads.render(g3d, 0x100);
        quads.setMeshVisible(MESH_COLLECTABLES, false);
        return;
    }
#endif
    if (mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        AnimPlayerQuad* anim = quads.getAnimPlayer(m_collectAnim);
        quads.setMeshVisible(m_collectMesh, anim->isAnimating());
        int count = m_player->getNumCollectablesCollected();
        m_showInverseCount = anim->isAnimatingBackward() ? count > 1 : count > 0;
        quads.setMeshVisible(MESH_COLLECTABLES_RIVAL, m_showInverseCount);
        quads.render(g3d, 0x100);
        quads.setMeshVisible(m_collectMesh, false);
    } else if (mode == LevelData::GAME_MODE_STORY) {
        quads.setMeshVisible(MESH_COLLECTABLES, m_collectHudTimer > 0);
    }
    quads.render(g3d, 0x100);
}

void GameScreen::initMenuMain() {
    if (!m_menu.isCreated()) {
        if (m_type == TYPE_SINGLE) {
            m_menu.createSingle();
        } else {
            m_menu.createSplit(m_type == TYPE_SPLIT_FIRST, m_height);
        }
        m_menu.setSelectionIndex(-1);
    }
    m_menu.wrapStrings(true);
    m_menu.reset();
    AppEngine::getCanvas()->getQuadManager().setMeshVisible(95, false);
}

void GameScreen::resetLevel() {
    m_camera->startSmoothTracking(m_player, true);
    m_checkpoint = NULL;
#ifdef MEDECOMP_PORT
    if (m_state == SCREEN_MULTIPLAYER_COMPLETE) {
        stateTransition(SCREEN_GAME);
    }
#endif
}

void GameScreen::renderRaceStart2D(midp::Graphics* g) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, m_countdownText, 41, m_width >> 1, 132, 66);
}

void GameScreen::initRaceStart() {
    m_raceStartTimer = 1000;
    switch (m_state) {
    case SCREEN_RACE_START_3:
        m_countdownText = new midp::String("3");
        break;
    case SCREEN_RACE_START_2:
        m_countdownText = new midp::String("2");
        break;
    case SCREEN_RACE_START_1:
        m_countdownText = new midp::String("1");
        break;
    default:
        break;
    }
    if (m_player->getPlayerIndex() == 0) {
        AppEngine::getCanvas()->getSoundManager().playEvent(37, 1.0f);
    }
}

void GameScreen::renderInverseHud2D(midp::Graphics* g) {
    if (!m_showInverseCount) {
        return;
    }
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    int count = m_player->getNumCollectablesCollected();
    int total = m_scene->getMap().getNumCollectablesTotal();
    int x = m_width - quads.getMeshX(MESH_COLLECTABLES_RIVAL);
    int y = m_height + 5 - quads.getMeshTop(MESH_COLLECTABLES_RIVAL);
    canvas->drawOfStatString(g, 8, 0x800, count, total, x, y, 10);
}

void GameScreen::renderHud2D(midp::Graphics* g) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    LevelData* levelData = AppEngine::getLevelData();
    int mode = levelData->getGameMode();
    if (mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        int time = m_scene->getRaceTime();
        if (time >= 0) {
            int font = (time <= 10000 && (time & 0x180) == 0) ? 43 : 42;
            canvas->drawStatString(g, font, 0x800, AppEngine::STAT_TIME, time, m_width - 170, 16, 9);
        }
#ifdef MEDECOMP_PORT
        if (m_type == TYPE_SINGLE) {
            if (m_scene->getState() == SceneGame::STATE_LEVEL_COMPLETE_MULTIPLAYER) {
                return;
            }
            int total = m_scene->getMap().getNumCollectablesTotal();
            int y = quads.getMeshBottom(MESH_COLLECTABLES) + 10;
            canvas->drawOfStatString(g, 8, 0x800, m_player->getNumCollectablesCollected(), total, m_width - 66, y, 10);
            if (port::net::rivalsNetwork()) {
                canvas->drawOfStatString(g, 8, TEXT_THEIR_BAGS, port::net::rivalsRemoteBags(), total, m_width - 20,
                                         y + 30, 12);
            }
            return;
        }
#endif
        if (m_showInverseCount) {
            int count = m_player->getNumCollectablesCollected();
            int total = m_scene->getMap().getNumCollectablesTotal();
            int x = quads.getMeshX(MESH_COLLECTABLES_RIVAL);
            int y = quads.getMeshBottom(MESH_COLLECTABLES_RIVAL) + 5;
            canvas->drawOfStatString(g, 8, 0x800, count, total, x, y, 10);
        }
    } else if (mode == LevelData::GAME_MODE_SPEEDRUN || mode == LevelData::GAME_MODE_CHALLENGE) {
        const Level* level = levelData->getCurrentLevelObject();
        int x = quads.getMeshRight(MESH_RACE_TIMER) + 10;
        int time = m_scene->getRaceTime();
        canvas->drawStatString(g, 12, 0x800, AppEngine::STAT_TIME, time, x, 10, 9);
#ifdef MEDECOMP_PORT
        if (medecomp_player_level_playing()) {
            int best = medecomp_player_level_best();
            if (best > 0) {
                canvas->drawStatString(g, 8, 0x800, AppEngine::STAT_TIME, best, x, 40, 9);
            }
            return;
        }
#endif
        if (level->getNumStarsAchieved() == 3) {
            canvas->drawStatString(g, 8, 0x800, AppEngine::STAT_TIME, level->getBestSpeedRunTimeMillis(), x, 40, 9);
        } else {
            int stars = level->getMinStarsWithTime(time);
            int required = level->getSpeedRunRequirementMillis(midp::Math::max(1, stars));
            canvas->drawStatString(g, 8, 0x800, AppEngine::STAT_TIME, required, x, 40, 9);
            quads.playAnimTo(1, stars);
        }
    } else if (mode == LevelData::GAME_MODE_STORY) {
        if (!quads.getMeshVisible(MESH_COLLECTABLES)) {
            return;
        }
        int y = quads.getMeshBottom(MESH_COLLECTABLES) + 10;
        int count = m_player->getNumCollectablesCollected();
        int total = m_scene->getMap().getNumCollectablesTotal();
        canvas->drawOfStatString(g, 7, 0x800, count, total, m_width - 66, y, 10);
    }
}

void GameScreen::renderState2D(midp::Graphics* g) {
    switch (m_state) {
    case SCREEN_GAME:
    case SCREEN_DEATH_FADE:
        renderHud2D(g);
        break;
    case SCREEN_RACE_START_3:
    case SCREEN_RACE_START_2:
    case SCREEN_RACE_START_1:
        renderHud2D(g);
        renderRaceStart2D(g);
        break;
    case SCREEN_MENU_MAIN:
        renderMenuMain2D(g);
        break;
    case SCREEN_SOUND_SETTINGS:
        m_soundSettings->render2D(g);
        break;
    case SCREEN_HELP_MENU:
        m_helpText->render2D(g);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        m_results->render2D(g);
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        m_confirmation->render2D(g);
        break;
    }
}

void GameScreen::renderState3D(midp::Graphics* g, m3g::Graphics3D* g3d) {
    switch (m_state) {
    case SCREEN_GAME:
    case SCREEN_RACE_START_3:
    case SCREEN_RACE_START_2:
    case SCREEN_RACE_START_1:
    case SCREEN_DEATH_FADE:
        renderHud3D(g3d);
        break;
    case SCREEN_MENU_MAIN:
        m_dimRect.render3D(g3d);
        renderMenuMain3D(g, g3d);
        break;
    case SCREEN_SOUND_SETTINGS:
        m_dimRect.render3D(g3d);
        m_soundSettings->render3D(g3d);
        break;
    case SCREEN_HELP_MENU:
        m_dimRect.render3D(g3d);
        m_helpText->render3D(g3d);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        m_results->render3D(g3d);
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        m_dimRect.render3D(g3d);
        m_confirmation->render3D(g3d);
        break;
    }
}

void GameScreen::fadeToBlack() {
    AppEngine::getCanvas()->getQuadManager().playAnim(m_fadeAnim, 0);
}

void GameScreen::initDeathFade() {
    if (m_type == TYPE_SINGLE) {
        SpywareManager::getInstance()->trackFaithKilled();
    }
    fadeToBlack();
}

void GameScreen::setCheckpointObject(GameObjectCheckpoint* checkpoint) {
    GameObjectCheckpoint* prev = m_checkpoint;
    m_checkpoint = checkpoint;
    if (prev != NULL) {
        prev->updateDishSelection();
    }
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        m_player->checkpointActivated();
    } else {
        m_scene->getMap().singlePlayerCheckpointActivated();
    }
    if (checkpoint == NULL) {
        return;
    }
    SpywareManager::getInstance()->trackCheckpointTriggered(m_scene->getMap().getUniqueGameObjectID(checkpoint));
    m_scene->checkpointReached();
}

void GameScreen::decNumCollectablesCollected() {
    m_player->decNumCollectablesCollected();
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        AnimPlayerQuad* anim = AppEngine::getCanvas()->getQuadManager().getAnimPlayer(m_collectAnim);
        anim->playAnim(anim->isAnimatingBackward() ? 0 : 2);
    }
}

void GameScreen::incNumCollectablesCollected() {
    m_player->incNumCollectablesCollected();
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        AnimPlayerQuad* anim = AppEngine::getCanvas()->getQuadManager().getAnimPlayer(m_collectAnim);
        anim->playAnimReverse(anim->isAnimatingForward() ? 0 : 2);
    }
    m_collectHudTimer = 4000;
}

void GameScreen::deinitDeathFade(ScreenState next) {
    if (next != SCREEN_GAME || AppEngine::getLevelData()->getGameMode() != LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        return;
    }
    if (m_player->getNumCollectablesCollected() < 1) {
        return;
    }
    m_player->decNumCollectablesCollected();
#ifdef MEDECOMP_PORT
    if (AppEngine::getCanvas()->getSceneGame()->numGameScreens() == 1) {
        port::net::rivalsGiveBag();
        return;
    }
#endif
    int rival = 1 - m_player->getPlayerIndex();
    AppEngine::getCanvas()->getSceneGame()->getGameScreen(rival)->incNumCollectablesCollected();
}

void GameScreen::stateTransition(ScreenState state) {
    ScreenState prev = m_state;
    if (isInGameState()) {
        m_resumeState = prev;
    }
    switch (prev) {
    case SCREEN_RACE_START_3:
    case SCREEN_RACE_START_2:
        m_countdownText = NULL;
        break;
    case SCREEN_RACE_START_1:
        m_countdownText = NULL;
        startFlashEffect(AppEngine::getCanvas()->getQuadManager());
        break;
    case SCREEN_DEATH_FADE:
        deinitDeathFade(state);
        break;
    case SCREEN_SOUND_SETTINGS:
        m_soundSettings = NULL;
        AppEngine::getCanvas()->saveRMSAppSettings();
        break;
    case SCREEN_HELP_MENU:
        m_helpText = NULL;
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        m_confirmation = NULL;
        break;
    default:
        break;
    }

    m_state = state;
    m_pointer1.release();
    m_pointer2.release();
    switch (state) {
    case SCREEN_RACE_START_3:
    case SCREEN_RACE_START_2:
    case SCREEN_RACE_START_1:
        initRaceStart();
        break;
    case SCREEN_DEATH_FADE:
        initDeathFade();
        break;
    case SCREEN_MENU_MAIN:
        initMenuMain();
        break;
    case SCREEN_SOUND_SETTINGS:
        m_soundSettings = new GameUI::SoundSettings(m_scene, this);
        break;
    case SCREEN_HELP_MENU:
        m_helpText = new GameUI::ScrollingText(m_scene, this, ITEM_HELP, 0x81e, 0x804);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        initMultiplayerComplete();
        break;
    case SCREEN_CONFIRM_RESTART:
        m_confirmation = new GameUI::Confirmation(0x90a, m_width, m_height, 0x805, 0x806);
        break;
    case SCREEN_CONFIRM_QUIT:
        m_confirmation = new GameUI::Confirmation(0x90b, m_width, m_height, 0x805, 0x806);
        break;
    default:
        break;
    }
}

void GameScreen::confirmResult(bool yes) {
    if (m_state == SCREEN_CONFIRM_QUIT) {
        if (yes) {
            SpywareManager::getInstance()->trackQuitLevel(AppEngine::getLevelData()->getCurrentLevelIndex());
#ifdef MEDECOMP_PORT
            port::net::raceUserQuit();
#endif
            m_scene->stateTransition(SceneGame::STATE_EXIT_LEVEL);
            AppEngine::getCanvas()->startFadeOut(true, 0xffffff);
            return;
        }
    } else if (m_state == SCREEN_CONFIRM_RESTART) {
        if (yes) {
#ifdef MEDECOMP_PORT
            port::net::raceUserRestart();
#endif
            m_scene->userRestartingLevel();
            return;
        }
    } else {
        return;
    }
    stateTransition(SCREEN_MENU_MAIN);
}

void GameScreen::pointerReleasedConfirmation(GameScreenPointer* pointer) {
    if (pointer != &m_pointer1) {
        return;
    }
    int result = m_confirmation->pointerReleased(pointer->m_pressX, pointer->m_pressY, pointer->m_x, pointer->m_y, this);
    if (result == GameUI::Confirmation::RESULT_NO) {
        confirmResult(false);
    } else if (result == GameUI::Confirmation::RESULT_YES) {
        confirmResult(true);
    }
}

void GameScreen::pointerReleasedHelpMenu(GameScreenPointer* pointer) {
    if (pointer != &m_pointer1) {
        return;
    }
    if (m_helpText->pointerReleased(pointer->m_pressX, pointer->m_pressY, pointer->m_x, pointer->m_y, pointer->m_id,
                                    this) == 1) {
        stateTransition(SCREEN_MENU_MAIN);
    }
}

void GameScreen::pointerReleasedSoundSettings(GameScreenPointer* pointer) {
    if (m_soundSettings->pointerReleased(m_pointer1.m_pressX, m_pointer1.m_pressY, m_pointer1.m_x, m_pointer1.m_y,
                                         m_pointer1.m_id) == 1) {
        stateTransition(SCREEN_MENU_MAIN);
    }
}

void GameScreen::selectMenuMain(int item) {
    switch (item) {
    case ITEM_HELP:
        stateTransition(SCREEN_HELP_MENU);
        break;
    case ITEM_GAME_OPTIONS:
        m_scene->stateTransition(SceneGame::STATE_GAME_OPTIONS);
        break;
    case ITEM_MEDIA_PICKER:
        m_scene->stateTransition(SceneGame::STATE_MEDIA_PICKER);
        break;
    case ITEM_RESUME:
        m_scene->resumeMenuPaused();
        break;
    case ITEM_RESTART:
        stateTransition(SCREEN_CONFIRM_RESTART);
        break;
    case ITEM_QUIT:
        stateTransition(SCREEN_CONFIRM_QUIT);
        break;
    case ITEM_SOUND_SETTINGS:
        stateTransition(SCREEN_SOUND_SETTINGS);
        break;
    }
}

void GameScreen::updateMenuMain(int dt) {
    m_menu.update(dt);
    if (!m_menu.isItemSelected()) {
        return;
    }
    selectMenuMain(m_menu.getSelectedSubMenu()->getSelectedItem());
    if (m_scene->getState() == SceneGame::STATE_MEDIA_PICKER) {
        return;
    }
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(49, false);
    quads.setMeshVisible(95, true);
}

void GameScreen::updateRaceStart(int dt) {
    m_raceStartTimer -= dt;
    if (m_raceStartTimer > 0) {
        return;
    }
    switch (m_state) {
    case SCREEN_RACE_START_3:
        stateTransition(SCREEN_RACE_START_2);
        break;
    case SCREEN_RACE_START_2:
        stateTransition(SCREEN_RACE_START_1);
        break;
    case SCREEN_RACE_START_1:
        stateTransition(SCREEN_GAME);
        AppEngine::getCanvas()->getSoundManager().playEvent(35, 1.0f);
        break;
    default:
        break;
    }
}

void GameScreen::menuPause() {
    stateTransition(SCREEN_MENU_MAIN);
}

void GameScreen::menuResume() {
    if (isInGameState()) {
        return;
    }
    stateTransition(m_resumeState);
}

void GameScreen::updateEffectAnims(QuadManager& quads, int dt) {
    if (isInGameState()) {
        quads.updateAnim(m_fadeAnim, dt);
    }
    quads.updateAnim(m_painAnim, dt);
    quads.updateAnim(m_flashAnim, dt);
    quads.updateAnim(m_collectAnim, dt);
    if (m_collectHudTimer > 0) {
        m_collectHudTimer -= dt;
    }
    if (m_painSoundHandle != -1 && !AppEngine::getCanvas()->getSoundManager().isHandlePlaying(m_painSoundHandle)) {
        m_painSoundHandle = -1;
    }
}

void GameScreen::startPainEffect(QuadManager& quads) {
    quads.playAnim(m_painAnim, 2);
    if (m_painSoundHandle != -1) {
        return;
    }
    int group;
    switch (m_player->getType()) {
    case 0:
        group = 1;
        break;
    case 1:
        group = 3;
        break;
    default:
        return;
    }
    int sound = m_scene->getSoundPoolManager().getRandomSoundEventID(1, group, -1);
    m_painSoundHandle = AppEngine::getCanvas()->getSoundManager().playEvent(sound, 1.0f);
}

void GameScreen::fadeFromBlack() {
    AppEngine::getCanvas()->getQuadManager().playAnimReverse(m_fadeAnim, 0);
}

void GameScreen::updateDeathFade(int dt) {
    if (AppEngine::getCanvas()->getQuadManager().isAnimating(m_fadeAnim)) {
        return;
    }
    if (AppEngine::getLevelData()->isMultiplayerMode()) {
        m_player->resetCheckpoint();
        m_camera->startSmoothTracking(m_player, true);
    } else {
        m_scene->resetCheckpoint();
    }
    fadeFromBlack();
    stateTransition(SCREEN_GAME);
}

void GameScreen::applyGestures() {
    if (m_inputFlags == 0) {
        return;
    }
    if ((m_inputFlags & (INPUT_SWIPE_LEFT | INPUT_SWIPE2_LEFT)) == (INPUT_SWIPE_LEFT | INPUT_SWIPE2_LEFT)) {
        m_player->addGesture(GameObjectPlayer::GESTURE_LEFT_TWO_FINGER);
    }
    if ((m_inputFlags & INPUT_SWIPE_LEFT) && !m_pointer2.isPressed()) {
        m_player->addGesture(GameObjectPlayer::GESTURE_LEFT);
    }
    if ((m_inputFlags & (INPUT_SWIPE_RIGHT | INPUT_SWIPE2_RIGHT)) == (INPUT_SWIPE_RIGHT | INPUT_SWIPE2_RIGHT)) {
        m_player->addGesture(GameObjectPlayer::GESTURE_RIGHT_TWO_FINGER);
    }
    if ((m_inputFlags & INPUT_SWIPE_RIGHT) && !m_pointer2.isPressed()) {
        m_player->addGesture(GameObjectPlayer::GESTURE_RIGHT);
    }
    if (m_inputFlags & INPUT_SWIPE_UP) {
        m_player->addGesture(GameObjectPlayer::GESTURE_UP);
    }
    if ((m_inputFlags & INPUT_SWIPE_DOWN) && !m_pointer2.isPressed()) {
        m_player->addGesture(GameObjectPlayer::GESTURE_DOWN);
    }
    if (m_inputFlags & INPUT_TAP) {
        m_player->addGesture(m_player->getTapGesture());
    }
    if (m_inputFlags & INPUT_HOLD) {
        m_player->addGesture(GameObjectPlayer::GESTURE_STOP);
    }
    if ((m_inputFlags & (INPUT_SWIPE_DOWN | INPUT_SWIPE2_DOWN)) == (INPUT_SWIPE_DOWN | INPUT_SWIPE2_DOWN)
#ifdef MEDECOMP_PORT
        && !(port::net::raceOpponent() && m_scene->getState() == SceneGame::STATE_RACE_START)
#endif
    ) {
        SpywareManager::getInstance()->trackGamePaused();
        m_scene->stateTransition(SceneGame::STATE_MENU_PAUSED);
    }
    m_inputFlags = 0;
}

void GameScreen::updatePointers(int dt) {
    if (m_pointer1.checkHold(dt)) {
        m_inputFlags |= INPUT_HOLD;
    }
}

bool GameScreen::pointerReleased(int id) {
    GameScreenPointer* pointer = getPointer(id);
    if (pointer == NULL) {
        return false;
    }
    switch (m_state) {
    case SCREEN_GAME:
        pointerReleasedGame(pointer);
        break;
    case SCREEN_MENU_MAIN:
        m_menu.pointerReleased(pointer->m_screenX, pointer->m_screenY, pointer->m_x, pointer->m_y, m_width, m_height,
                               this);
        break;
    case SCREEN_SOUND_SETTINGS:
        pointerReleasedSoundSettings(pointer);
        break;
    case SCREEN_HELP_MENU:
        pointerReleasedHelpMenu(pointer);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        pointerReleasedMultiplayerComplete(pointer);
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        pointerReleasedConfirmation(pointer);
        break;
    default:
        break;
    }
    pointer->release();
    return true;
}

void GameScreen::updateCamera(int dt) {
    if (dt > 0) {
        switch (m_type) {
        case TYPE_SINGLE:
            m_camera->update(dt, 90.0f);
            break;
        case TYPE_SPLIT_FIRST:
#ifdef MEDECOMP_SPLIT_STACKED
            m_camera->update(dt, 90.0f);
            break;
#endif
            m_camera->update(dt, 180.0f);
            break;
        case TYPE_SPLIT_SECOND:
#ifdef MEDECOMP_SPLIT_STACKED
            m_camera->update(dt, 90.0f);
            break;
#endif
            m_camera->update(dt, 0.0f);
            break;
        }
    }
    MathVector from(m_camera->getLookFromX(), m_camera->getLookFromY(), m_camera->getLookFromZ());
    MathVector at(m_camera->getLookAtX(), m_camera->getLookAtY(), m_camera->getLookAtZ());
#ifdef MEDECOMP_PORT
    if (m_type == TYPE_SINGLE) {
        m_frustum.set(from, at, m_camera->getNearClip(), m_camera->getFarClip(), medecomp_wide_fov(m_camera->getFOV()),
                      m_width + 2 * medecomp_wide_extra(), m_height);
    } else
#endif
#ifdef MEDECOMP_SPLIT_STACKED
    {
        float vfov = m_camera->getFOV() * 0.5f;
        float hfov = medecomp_hor_fov(vfov, m_width + 2.0f * medecomp_wide_extra(), m_height);
        m_frustum.set(from, at, m_camera->getNearClip(), m_camera->getFarClip(), hfov, hfov, vfov);
    }
#else
    m_frustum.set(from, at, m_camera->getNearClip(), m_camera->getFarClip(), m_camera->getFOV(), m_width, m_height);
#endif
    if (m3g::g_usingGLES20) {
        m_m3gCamera->getTranslation(m_cameraPos);
        AppEngine::getCanvas()->getReflectionShader()->setCameraPos(m_cameraPos[0], m_cameraPos[1], m_cameraPos[2]);
    }
}

void GameScreen::calcGlobalCoordinates(int localX, int localY, int& x, int& y) {
#ifdef MEDECOMP_SPLIT_STACKED
    x = localX;
    y = m_type == TYPE_SPLIT_SECOND ? localY + m_height + 10 : localY;
    return;
#endif
    switch (m_type) {
    case TYPE_SPLIT_SECOND: {
        int canvasHeight = AppEngine::getCanvas()->getHeight();
        x = localY + m_height + 10;
        y = canvasHeight - localX;
        break;
    }
    case TYPE_SPLIT_FIRST:
        x = m_height - localY;
        y = localX;
        break;
    case TYPE_SINGLE:
        x = localX;
        y = localY;
        break;
    }
}

bool GameScreen::calcLocalCoordinates(int x, int y, int& localX, int& localY) {
#ifdef MEDECOMP_SPLIT_STACKED
    localX = x;
    switch (m_type) {
    case TYPE_SPLIT_SECOND:
        localY = y - (m_height + 10);
        return y >= m_height + 10;
    case TYPE_SPLIT_FIRST:
        localY = y;
        return y <= m_height;
    default:
        localY = y;
        return true;
    }
#endif
    switch (m_type) {
    case TYPE_SPLIT_SECOND: {
        int canvasHeight = AppEngine::getCanvas()->getHeight();
        localX = canvasHeight - y;
        localY = x - (m_height + 10);
        return x >= m_height + 10;
    }
    case TYPE_SPLIT_FIRST:
        localX = y;
        localY = m_height - x;
        return x <= m_height;
    case TYPE_SINGLE:
        localX = x;
        localY = y;
        break;
    }
    return true;
}

bool GameScreen::pointerDragged(int x, int y, int id, int canvasWidth, int canvasHeight) {
    GameScreenPointer* pointer = getPointer(id);
    if (pointer == NULL) {
        return false;
    }
    int localX = 0;
    int localY = 0;
    if (!calcLocalCoordinates(x, y, localX, localY)) {
        return false;
    }

    int dx = localX - pointer->m_x;
    int dy = localY - pointer->m_y;
    if (iabs(dx) <= iabs(dy)) {
        dx = 0;
    }
    if (iabs(dy) <= iabs(dx)) {
        dy = 0;
    }
    int bit = (pointer == &m_pointer2) ? INPUT_SWIPE2_LEFT : INPUT_SWIPE_LEFT;
    int dir = pointer->m_swipeDir;
    bool moved = true;
    if (dx <= -SWIPE_MIN_DIST) {
        if (dy >= -SWIPE_MAX_SIDE && dy <= SWIPE_MAX_SIDE && dir != GameScreenPointer::SWIPE_LEFT &&
            dir != GameScreenPointer::SWIPE_UP) {
            m_inputFlags |= bit;
            dir = GameScreenPointer::SWIPE_LEFT;
        }
    } else if (dx >= SWIPE_MIN_DIST) {
        if (dy >= -SWIPE_MAX_SIDE && dy <= SWIPE_MAX_SIDE && dir != GameScreenPointer::SWIPE_RIGHT &&
            dir != GameScreenPointer::SWIPE_UP) {
            m_inputFlags |= bit << 1;
            dir = GameScreenPointer::SWIPE_RIGHT;
        }
    } else if (dy <= -SWIPE_MIN_DIST) {
        if ((float)dx >= -30.0f && (float)dx <= 30.0f && dir != GameScreenPointer::SWIPE_UP) {
            m_inputFlags |= bit << 2;
            dir = GameScreenPointer::SWIPE_UP;
        }
    } else if (dy >= SWIPE_MIN_DIST) {
        if ((float)dx >= -30.0f && (float)dx <= 30.0f) {
            if (dir != GameScreenPointer::SWIPE_UP && dir != GameScreenPointer::SWIPE_DOWN) {
                m_inputFlags |= bit << 3;
            }
            dir = GameScreenPointer::SWIPE_DOWN;
        }
    } else {
        moved = false;
    }
    if (moved) {
        pointer->swipe(dir, x, y, localX, localY);
    }

    switch (m_state) {
    case SCREEN_MENU_MAIN:
        m_menu.pointerDragged(x, y, localX, localY, m_width, m_height, this);
        break;
    case SCREEN_SOUND_SETTINGS:
        m_soundSettings->pointerDragged(m_pointer1.m_pressX, m_pointer1.m_pressY, localX, localY, id);
        break;
    case SCREEN_HELP_MENU:
        m_helpText->pointerDragged(m_pointer1.m_pressX, m_pointer1.m_pressY, localX, localY, id);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        if (pointer == &m_pointer1) {
            m_results->pointerDragged(m_pointer1.m_pressX, m_pointer1.m_pressY, localX, localY);
        }
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        if (pointer == &m_pointer1) {
            m_confirmation->pointerDragged(m_pointer1.m_pressX, m_pointer1.m_pressY, localX, localY);
        }
        break;
    default:
        break;
    }
    return true;
}

bool GameScreen::pointerPressed(int x, int y, int id, int canvasWidth, int canvasHeight) {
    int localX = 0;
    int localY = 0;
    if (!calcLocalCoordinates(x, y, localX, localY)) {
        return false;
    }
    if (!m_pointer1.isPressed()) {
        m_pointer1.press(id, x, y, localX, localY);
        m_pointer2.release();
    } else if (!m_pointer2.isPressed()) {
        m_pointer2.press(id, x, y, localX, localY);
    }
    switch (m_state) {
    case SCREEN_MENU_MAIN:
        m_menu.pointerPressed(x, y, localX, localY, m_width, m_height, this);
        break;
    case SCREEN_SOUND_SETTINGS:
        m_soundSettings->pointerPressed(localX, localY, id);
        break;
    case SCREEN_HELP_MENU:
        m_helpText->pointerPressed(localX, localY, id);
        break;
    case SCREEN_MULTIPLAYER_COMPLETE:
        m_results->pointerPressed(localX, localY);
        break;
    case SCREEN_CONFIRM_RESTART:
    case SCREEN_CONFIRM_QUIT:
        m_confirmation->pointerPressed(localX, localY);
        break;
    default:
        break;
    }
    return true;
}

void GameScreen::getScreenLocalPositionNoZCheck(const MathVector& pos, int& x, int& y) const {
    float fx = 0.0f;
    float fy = 0.0f;
    m_frustum.getScreenPositionFractionNoZCheck(pos, fx, fy);
    x = (int)(m_width * fx);
    y = (int)(m_height * fy);
}

void GameScreen::getScreenLocalPosition(const MathVector& pos, int& x, int& y) const {
    float fx = 0.0f;
    float fy = 0.0f;
    if (m_frustum.getScreenPositionFraction(pos, fx, fy)) {
        x = (int)(m_width * fx);
        y = (int)(m_height * fy);
    }
}

void GameScreen::update(int dt) {
    switch (m_state) {
    case SCREEN_RACE_START_3:
    case SCREEN_RACE_START_2:
    case SCREEN_RACE_START_1:
        updateRaceStart(dt);
        break;
    case SCREEN_DEATH_FADE:
        updateDeathFade(dt);
        break;
    case SCREEN_MENU_MAIN:
        updateMenuMain(dt);
        break;
    case SCREEN_HELP_MENU:
        m_helpText->update(dt);
        break;
    default:
        break;
    }
}

void GameScreen::setupLocatorQuadForRender(GameObjectPlayer* player) {
#ifdef MEDECOMP_PORT
    setupLocatorQuad(player->m_position);
}

void GameScreen::setupLocatorQuad(const MathVector& at) {
#endif
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int width = m_width;
    int height = m_height;
    int x = 0;
    int y = 0;
#ifdef MEDECOMP_PORT
    MathVector pos(at);
#else
    MathVector pos(player->m_position);
#endif
    pos.y += 0.9f;
    float alpha = 1.0f;
    getScreenLocalPositionNoZCheck(pos, x, y);

    int dx = x - (width >> 1);
    int dy = y - (height >> 1);
    float halfWidth = width >> 1;
    float halfHeight = height >> 1;
    float fx = dx / halfWidth;
    float fy = dy / halfHeight;
    float dist = fx * fx + fy * fy;
    if (dist < 6.0f) {
        if (dist < 0.6f) {
            alpha = 0.0f;
        } else {
            alpha = (dist - 0.6f) * (1.0f / 5.4f);
        }
    }
    float angle = MathTrig::calcAngleFromUpToVectorDeg(dx, dy);
    calculateVectorToScreenSide(dx, dy);
    float posX = halfWidth + dx * 0.9f;
    float posY = halfHeight + dy * 0.9f;
    float size = ((1.0f - alpha) * (1.0f - alpha) * -0.5f + 1.0f) * 64.0f;
    quads.setMeshRotation(MESH_LOCATOR, angle + 90.0f);
    quads.setMeshBounds(MESH_LOCATOR, posX, posY, size, size, 18);
    quads.setMeshAlpha(MESH_LOCATOR, alpha);
}

void GameScreen::render(midp::Graphics* g) {
    AppEngine* canvas = AppEngine::getCanvas();
    QuadManager& quads = canvas->getQuadManager();
    int canvasWidth = canvas->getWidth();
    int canvasHeight = canvas->getHeight();

    m3g::Node* skyDome = m_scene->getMap().getSkyDomeNode();
    if (skyDome != NULL) {
        const MathVector& focal = m_frustum.getFocalPoint();
        skyDome->setTranslation(focal.x, focal.y, focal.z);
    }
    if (m_world == NULL) {
        return;
    }

    bool inGame = isInGameState();
    int viewX = 0;
    int viewWidth = canvasWidth;
#ifdef MEDECOMP_SPLIT_STACKED
    int viewY = 0;
    int viewHeight = canvasHeight;
    int worldX = 0;
    int worldWidth = canvasWidth;
    if (m_type != TYPE_SINGLE) {
        viewHeight = m_height;
        if (m_type == TYPE_SPLIT_SECOND) {
            viewY = m_height + 10;
        }
        worldX = -medecomp_wide_extra();
        worldWidth = canvasWidth + 2 * medecomp_wide_extra();
        quads.setViewport(0, 0, canvasWidth, viewHeight, 1);
        m_dimRect.setBounds(worldX, 0, worldWidth, m_height);
    }
#else
    if (m_type == TYPE_SPLIT_SECOND) {
        viewWidth = (canvasWidth - 10) >> 1;
        viewX = viewWidth + 10;
        quads.setViewport(viewX, 0, viewWidth, canvasHeight, 0);
    } else if (m_type == TYPE_SPLIT_FIRST) {
        viewWidth = (canvasWidth - 10) >> 1;
        quads.setViewport(0, 0, viewWidth, canvasHeight, 2);
    }
    const int viewY = 0;
    const int viewHeight = canvasHeight;
    const int worldX = viewX;
    const int worldWidth = viewWidth;
#endif
    m_world->setActiveCamera(m_m3gCamera);

    float damage = 1.0f - m_player->getHealthFraction();
    if (damage != 0.0f) {
        quads.setMeshAlpha(m_damageMesh, damage);
        quads.setMeshVisible(m_damageMesh, true);
    }
    g->setClip(worldX, viewY, worldWidth, viewHeight);
    m3g::Graphics3D* g3d = canvas->getGraphics3D();
    g3d->bindTarget(g);
    g3d->setViewport(worldX, viewY, worldWidth, viewHeight);
    g3d->render(m_world);
#ifdef MEDECOMP_PORT
    if (m_type == TYPE_SINGLE) {
        const m3g::Camera* camera = m_m3gCamera;
        medecomp_post_world(camera->m_fovy, camera->m_aspect, camera->m_nearPlane, camera->m_farPlane);
    }
#endif

    midp::GraphicsIPhoneGLES* gles = static_cast<midp::GraphicsIPhoneGLES*>(g);
#ifdef MEDECOMP_SPLIT_STACKED
    if (m_type == TYPE_SPLIT_SECOND) {
        gles->setViewport(0, viewY, 0);
    }
#else
    if (m_type == TYPE_SPLIT_SECOND) {
        gles->setViewport(viewX, canvasHeight, -1);
    } else if (m_type == TYPE_SPLIT_FIRST) {
        gles->setViewport(viewWidth, 0, 1);
    }
#endif
    if (!inGame) {
        quads.render(g3d, m_effectScope);
        renderState3D(g, g3d);
        g3d->releaseTarget();
        renderState2D(g);
    } else {
        renderState3D(g, g3d);
        g3d->releaseTarget();
        renderState2D(g);
        g3d->bindTarget(g);
        g3d->setViewport(worldX, viewY, worldWidth, viewHeight);
        quads.render(g3d, m_effectScope);
        g3d->releaseTarget();
#ifndef MEDECOMP_SPLIT_STACKED
        if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
            if (m_type == TYPE_SPLIT_SECOND) {
                gles->setViewport(canvasWidth, 0, 1);
                renderInverseHud2D(g);
            } else if (m_type == TYPE_SPLIT_FIRST) {
                gles->setViewport(0, canvasHeight, -1);
                renderInverseHud2D(g);
            }
        }
#endif
    }
    gles->clearViewport();
    quads.setMeshVisible(m_damageMesh, false);

    if (m_type != TYPE_SINGLE) {
        g->setClip(0, 0, canvasWidth, canvasHeight);
        quads.clearViewport();
        if (m_type == TYPE_SPLIT_SECOND) {
            g->setColor(0xff000000);
#ifdef MEDECOMP_SPLIT_STACKED
            g->fillRect(-medecomp_wide_extra(), viewY - 10, canvasWidth + 2 * medecomp_wide_extra(), 10);
#else
            g->fillRect(viewX - 10, 0, 10, canvasHeight);
#endif
        }
    }
}

GameScreen::~GameScreen() {}

bool GameScreen::isInGameState() {
    return m_state == SCREEN_GAME || m_state == SCREEN_DEATH_FADE || isInRaceStart();
}

void GameScreen::startFlashEffect(QuadManager& quads) {
    quads.playAnim(m_flashAnim, 2);
}

int GameScreen::getLocalWidth() {
    return m_width;
}

int GameScreen::getLocalHeight() {
    return m_height;
}
