#include "game/SceneGame.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "ea_online/FBConnection.h"
#include "ea_online/Mayhem/GetUserRequest.h"
#include "ea_online/Mayhem/PostChallenge.h"
#include "ea_online/Mayhem/PostStatRequest.h"
#include "ea_online/Mayhem/PostUserRequest.h"
#include "ea_online/Mayhem/Token.h"
#include "ea_online/Mayhem/UserRequest.h"
#include "ea_online/SpywareManager.h"
#include "game/AchievementData.h"
#include "game/AnimationBlenderData.h"
#include "game/AppEngine.h"
#include "game/BGMusic.h"
#include "game/DataCameraAnim.h"
#include "game/GameCamera.h"
#include "game/GameCommon.h"
#include "game/GameObjectGhost.h"
#include "game/GameObjectPlayer.h"
#include "game/GameObjectRunnerData.h"
#include "game/GameScreen.h"
#include "game/GhostAnimation.h"
#include "game/GhostAnimationPlayback.h"
#include "game/GhostAnimationRecorder.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MEdgeMap.h"
#include "game/MediaPicker.h"
#include "game/ModelSet.h"
#include "game/QuadManager.h"
#include "game/ReflectionMapShader.h"
#include "game/ResourceManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "game/UI/NetworkWaitEffect.h"
#include "game/UI/PleaseWaitWindow.h"
#include "game/UI/PopupBox.h"
#include "game/UI/SoundOptions.h"
#include "game/UI/WindowStore.h"
#ifdef MEDECOMP_PORT
#include "port/net.h"
extern "C" int medecomp_solo_bags(void);
extern "C" void medecomp_solo_bags_result(int collected, int total, int timeLeft);
extern "C" int medecomp_player_level_test(void);
extern "C" int medecomp_player_level_playing(void);
extern "C" int medecomp_player_level_no_board(void);
#endif
#include "m3g/Appearance.h"
#include "m3g/AppearanceBase.h"
#include "m3g/Background.h"
#include "m3g/Graphics3D.h"
#include "m3g/Image2D.h"
#include "m3g/Mesh.h"
#include "m3g/OpenGLES20ShaderSupport.h"
#include "m3g/Texture2D.h"
#include "m3g/Transform.h"
#include "m3g/World.h"
#include "m3g/m3g.h"
#include "runtime/midp/AccelerometerIPhone.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/InputStream.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"

static const int TEXT_NONE = 0x800;

#ifdef MEDECOMP_PORT
#define TOP_MESSAGE2_HIDDEN_X (-300.0f - (float)medecomp_wide_extra())
#else
static const float TOP_MESSAGE2_HIDDEN_X = -300.0f;
#endif
static const float TOP_MESSAGE2_SHOWN_X = 10.0f;

static const int CUTSCENE_TARGET_PLAYER = 1;

static const int SOUND_GO = 35;
static const int SOUND_COUNTDOWN = 37;
static const int SOUND_TIME_OVER = 67;

enum WindowResult {
    WINDOW_RESULT_OK = 1,
    WINDOW_RESULT_CANCEL = 2,
};

enum MayhemStep {
    MAYHEM_FB_LOGIN = 1,
    MAYHEM_TOKEN = 2,
    MAYHEM_GET_USER = 3,
    MAYHEM_POST_USER = 4,
    MAYHEM_POST_STAT = 5,
    MAYHEM_POST_CHALLENGE = 6,
    MAYHEM_CLOSE = 7,
    MAYHEM_DONE = 8,
};

enum MayhemText {
    TEXT_SCORE_UPLOADED = 0x94c,
    TEXT_UPLOAD_ERROR = 0x94d,
    TEXT_SCORE_NOT_HIGHER = 0x96d,
    TEXT_CHALLENGE_UPLOADED = 0x96e,
    TEXT_CHALLENGE_ERROR = 0x96f,
    TEXT_TIME_TOO_LONG = 0x970,
};

static const int TEXT_LOADING = 0x812;
static const int TEXT_TAP_TO_CONTINUE = 0x813;

enum LoadingStep {
    LOADING_COLLECT_TEXTURES = 7,
    LOADING_WARM_SHADERS = 8,
    LOADING_RENDER_QUADS = 11,
    LOADING_UPLOAD_TEXTURES = 12,
    LOADING_DONE = 13,
};

float SceneGame::getAccelerometerTiltVelocity() const {
    return m_tiltVelocity;
}

void SceneGame::startCombatTime() {
    m_combatTime = 1750;
}

bool SceneGame::isCheckPointActive(GameObjectCheckpoint* checkpoint) {
    int count = m_gameScreens.m_length;
    for (int i = 0; i != count; i++) {
        if (m_gameScreens[i]->getCheckpointObject() == checkpoint) {
            return true;
        }
    }
    return false;
}

void SceneGame::addPlayerObject(GameObjectPlayer* player) {
    int count = m_gameScreens.m_length;
    for (int i = 0; i != count; i++) {
        if (m_gameScreens[i]->getPlayerObject() == NULL) {
            m_gameScreens[i]->setPlayerObject(player);
            return;
        }
    }
}

GameScreen* SceneGame::getScreenForPlayer(GameObjectPlayer* player) {
    int count = m_gameScreens.m_length;
    for (int i = 0; i != count; i++) {
        if (m_gameScreens[i]->getPlayerObject() == player) {
            return m_gameScreens[i];
        }
    }
    return NULL;
}

bool SceneGame::uploadQueueExhausted() {
    return m_uploadQueue.size() == 0;
}

const char* SceneGame::LAST_GHOST_ANIMATION_FILENAME = "lastGhostAnimation";

void SceneGame::uploadChallenge() {
    midp::ReferenceCountedPointer<midp::InputStream> stream(
        midp::Class::getResourceAsStream(new midp::String(LAST_GHOST_ANIMATION_FILENAME)));
    if (stream == NULL) {
        return;
    }
    midp::array<signed char> data(stream->size());
    stream->read(data);
    char statType[20];
    snprintf(statType, sizeof(statType), "challenge_level%d",
             AppEngine::getLevelData()->getCurrentLevelIndex() + 1);
    m_postChallenge = new Mayhem::PostChallenge(m_engine->m_mayhemToken.unsafeGetWrappedPointer(),
                                                m_engine->m_mayhemId, statType, m_challengeUserId, getRaceTime(),
                                                m_challengeComment, data.m_length, data.data());
}

bool SceneGame::uploadSpeedrunStat(bool) {
    int score = m_engine->timeToMayhemScore(m_raceTime, AppEngine::getLevelData()->getCurrentLevelIndex());
    if (score <= 0) {
        m_postStatRequest = NULL;
        return false;
    }
    midp::ReferenceCountedPointer<midp::InputStream> stream(
        midp::Class::getResourceAsStream(new midp::String(GhostAnimation::getCurrentLevelGhostFilename())));
    if (stream == NULL) {
        return false;
    }
    midp::array<signed char> data(stream->size());
    stream->read(data);
    char statType[20];
    snprintf(statType, sizeof(statType), "highscore_level%d",
             AppEngine::getLevelData()->getCurrentLevelIndex() + 1);
    m_postStatRequest = new Mayhem::PostStatRequest(m_engine->m_mayhemToken.unsafeGetWrappedPointer(),
                                                    m_engine->m_mayhemId, statType, score, data.m_length,
                                                    data.data());
    return true;
}

void SceneGame::initAbout() {
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_ABOUT, 0, 0, 0, 0);
}

void SceneGame::initHelp() {
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_HELP, 0, 0, 0, 0);
}

void SceneGame::initGameOptions() {
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_GAME_OPTIONS, 0, 0, 0, 0);
}

void SceneGame::initSoundOptions() {
    m_soundOptionsWindow = static_cast<UI::SoundOptions*>(
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_SOUND_OPTIONS, 0, 0, 0, 0));
}

void SceneGame::initMayhemWait() {
    m_pleaseWaitWindow = static_cast<UI::PleaseWaitWindow*>(
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_PLEASE_WAIT, 0, 0, 0, 0));
    if (m_mayhemStep == MAYHEM_POST_CHALLENGE) {
        m_mayhemPendingStep = MAYHEM_POST_CHALLENGE;
        if (m_engine->IsMayhemSessionValid()) {
            uploadChallenge();
            return;
        }
        FBConnection* fb = NULL;
        m_userRequest = new Mayhem::GetUserRequest(fb->m_userId != 0);
        m_mayhemStep = MAYHEM_GET_USER;
    } else if (m_mayhemStep == MAYHEM_POST_STAT && !m_mayhemResetSession) {
        if (!m_engine->IsMayhemSessionValid()) {
            m_mayhemPendingStep = MAYHEM_POST_STAT;
            if (!m_engine->IsMayhemSessionValid()) {
                m_userRequest = new Mayhem::GetUserRequest(true);
                m_mayhemStep = MAYHEM_GET_USER;
                return;
            }
        }
        if (!uploadSpeedrunStat(false)) {
            m_pleaseWaitWindow->Succeeded(TEXT_TIME_TOO_LONG);
            m_mayhemStep = MAYHEM_DONE;
            m_engine->getWindowStore().getNetworkWaitEffect()->stop();
        }
    }
}

void SceneGame::resume() {
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->resume();
    }
}

void SceneGame::initMediaPicker() {
    midp::Display* display = midp::Display::getDisplay(m_engine->getMIDlet());
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->suspend();
    }
    m_engine->getMediaPicker()->activate(display);
}

void SceneGame::setGameTimeFactor(float factor) {
    m_gameTimeFactor.setTargetValue(factor);
}

void SceneGame::clearGameTimeFactor() {
    m_gameTimeFactor.setTargetValue(1.0f);
    m_gameTimeFactor.setSteadyState(1.0f);
}

float SceneGame::getGameTimeFactor() {
    return m_gameTimeFactor.getFilteredValue();
}

float SceneGame::getAccelerometerTilt() {
    return m_tiltFilter.getFilteredValue();
}

void SceneGame::renderTopMessage(midp::Graphics* g) {
    if (m_topMessageText != TEXT_NONE) {
        float x = m_topMessageAnim.getCurrentValue();
        m_engine->getTextManager()->drawString(g, m_topMessageText, 16, (int)x, 50, 66);
    }
    if (m_topMessageText2 != TEXT_NONE) {
        float x = m_topMessageAnim2.getCurrentValue();
        int y = m_engine->getHeight() - 20;
        m_engine->getTextManager()->drawString(g, m_topMessageText2, 17, (int)x, y, 65);
    }
}

void SceneGame::updateTopMessage(int dt) {
    int width = m_engine->getWidth();
    if (m_topMessageText == TEXT_NONE) {
        return;
    }
    m_topMessageAnim.update(dt);
    m_topMessageAnim2.update(dt);
    if (!m_topMessageAnim.isFinished()) {
        return;
    }
    int half = width >> 1;
#ifdef MEDECOMP_PORT
    const int gone = half + medecomp_wide_extra();
#else
    const int gone = half;
#endif
    switch (m_topMessagePhase) {
    case 0:
        m_topMessageAnim.start(half, half, 5000, InterpolationTimed::LINEAR);
        m_topMessageAnim2.start(TOP_MESSAGE2_HIDDEN_X, TOP_MESSAGE2_HIDDEN_X, 5000, InterpolationTimed::LINEAR);
        break;
    case 1:
        m_topMessageAnim.start(half, -gone, 1000, InterpolationTimed::EASE_IN_QUAD);
        m_topMessageAnim2.start(TOP_MESSAGE2_HIDDEN_X, TOP_MESSAGE2_SHOWN_X, 1000, InterpolationTimed::EASE_OUT_QUAD);
        break;
    case 2:
        m_topMessageAnim.start(-gone, -gone, 5000, InterpolationTimed::LINEAR);
        m_topMessageAnim2.start(TOP_MESSAGE2_SHOWN_X, TOP_MESSAGE2_SHOWN_X, 5000, InterpolationTimed::LINEAR);
        break;
    case 3:
        m_topMessageAnim.start(-gone, -gone, 1000, InterpolationTimed::LINEAR);
        m_topMessageAnim2.start(TOP_MESSAGE2_SHOWN_X, TOP_MESSAGE2_HIDDEN_X, 1000, InterpolationTimed::EASE_IN_QUAD);
        break;
    case 4:
        m_topMessageText = TEXT_NONE;
        m_topMessageText2 = TEXT_NONE;
        break;
    }
    m_topMessagePhase++;
}

void SceneGame::startTopMessage(int text, int text2) {
    int width = m_engine->getWidth();
    m_topMessageText = text;
    m_topMessageText2 = text2;
    m_topMessagePhase = 0;
#ifdef MEDECOMP_PORT
    m_topMessageAnim.start(width + (width >> 1) + medecomp_wide_extra(), width >> 1, 1000, InterpolationTimed::EASE_OUT_QUAD);
#else
    m_topMessageAnim.start(width + (width >> 1), width >> 1, 1000, InterpolationTimed::EASE_OUT_QUAD);
#endif
    m_topMessageAnim2.start(TOP_MESSAGE2_HIDDEN_X, TOP_MESSAGE2_HIDDEN_X, 1000, InterpolationTimed::LINEAR);
}

void SceneGame::playCutscene(int cutscene) {
    DataCameraAnim* anim = m_map.getCutscene(cutscene);
    if (anim->getTargetID() == CUTSCENE_TARGET_PLAYER) {
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            GameScreen* screen = m_gameScreens[i];
            screen->getGameCamera()->playDataAnim(anim, screen->getPlayerObject());
        }
    } else {
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            m_gameScreens[i]->getGameCamera()->playDataAnim(anim, NULL);
        }
    }
}

void SceneGame::playerHurt(GameObjectPlayer* player, bool) {
    GameScreen* screen = m_gameScreens[player->getPlayerIndex()];
    screen->startPainEffect(AppEngine::getCanvas()->getQuadManager());
}

void SceneGame::checkpointReached() {
    m_engine->getSoundManager().playEvent(71, 1.0f);
}

void SceneGame::startIntroCam() {
#ifdef MEDECOMP_PORT
    if (medecomp_player_level_test()) {
        return;
    }
#endif
    LevelData* levelData = AppEngine::getLevelData();
    const Level* level = levelData->getCurrentLevelObject();
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        int anim = level->getIntroCamAnim(i);
        if (anim == -1) {
            continue;
        }
        GameScreen* screen = m_gameScreens[i];
        int facing = 1;
        if (levelData->isMultiplayerMode()) {
            facing = screen->getPlayerObject()->getFacingDir();
            if (anim == 146) {
                facing = -facing;
            }
        }
        screen->getGameCamera()->startCameraAnimation(anim, true, facing);
    }
}

void SceneGame::pointerReleasedGame(int x, int y, int id) {
    switch (m_state) {
    case STATE_GAME:
    case STATE_LEVEL_COMPLETE_MULTIPLAYER:
    case STATE_MENU_PAUSED:
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            if (m_gameScreens[i]->pointerReleased(id)) {
                return;
            }
        }
        break;
    default:
        break;
    }
}

void SceneGame::pointerDraggedGame(int x, int y, int id) {
    switch (m_state) {
    case STATE_GAME:
    case STATE_LEVEL_COMPLETE_MULTIPLAYER:
    case STATE_MENU_PAUSED:
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            if (m_gameScreens[i]->pointerDragged(x, y, id, m_engine->getWidth(), m_engine->getHeight())) {
                return;
            }
        }
        break;
    case STATE_LOADING:
    case STATE_FINISHED_GAME_STORY:
        if (!m_engine->isFading()) {
            m_loadingScrolled = true;
            float delta = ((float)y - m_loadingScrollY) * 0.002f;
            float scroll = m_engine->getLoadingScrollValue();
            m_engine->setScrollingLoad(false);
            m_engine->setLoadingScrollValue(scroll + delta);
        }
        m_loadingScrollY = (float)y;
        break;
    default:
        break;
    }
}

void SceneGame::pointerDragged(int x, int y, int id) {
    pointerDraggedGame(x, y, id);
}

void SceneGame::pointerReleased(int x, int y, int id) {
    if (m_state == STATE_FINISHED_GAME_STORY) {
        m_engine->setScrollingLoad(true);
        if (m_finishedStoryTimer <= 0 && !m_engine->isFading() && !m_engine->isFadedOut() &&
            !m_loadingScrolled) {
            m_engine->startFadeOut(true, 0xffffff);
        }
    } else if (m_state == STATE_LOADING) {
        m_engine->setScrollingLoad(true);
        if (m_loadingProgress == 100 && !m_engine->isFading() && !m_engine->isFadedOut() &&
            !m_loadingScrolled) {
            m_engine->startFadeOut(true, 0xffffff);
        }
        return;
    }
    pointerReleasedGame(x, y, id);
}

void SceneGame::pointerPressedGame(int x, int y, int id) {
    switch (m_state) {
    case STATE_GAME:
    case STATE_LEVEL_COMPLETE_MULTIPLAYER:
    case STATE_MENU_PAUSED:
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            if (m_gameScreens[i]->pointerPressed(x, y, id, m_engine->getWidth(), m_engine->getHeight())) {
                return;
            }
        }
        break;
    case STATE_LOADING:
    case STATE_FINISHED_GAME_STORY:
        m_loadingScrolled = false;
        m_loadingScrollY = (float)y;
        break;
    default:
        break;
    }
}

void SceneGame::pointerPressed(int x, int y, int id, int) {
    pointerPressedGame(x, y, id);
}

void SceneGame::deinitFinishedGameStory() {
    m_engine->deinitLoadingScreen();
}

void SceneGame::renderFinishedGameStory(midp::Graphics* g) {
    m_engine->renderLoading(g);
    if (m_finishedStoryTimer <= 0) {
        TextManager* text = m_engine->getTextManager();
        int x = m_engine->getWidth() >> 1;
        int y = m_engine->getHeight() - 6 - text->getLineHeight(7);
        text->drawString(g, TEXT_TAP_TO_CONTINUE, 7, x, y, 10);
    }
}

void SceneGame::updateFinishedGameStory(int dt) {
    if (m_engine->isFadedOut()) {
        m_gameActive = false;
        m_engine->changeScene(2, 3);
    }
    m_finishedStoryTimer -= dt;
    m_engine->updateLoading(dt);
}

void SceneGame::initFinishedGameStory() {
    m_engine->initLoadingScreen(14);
    m_finishedStoryTimer = 3000;
}

void SceneGame::initRaceStart() {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->stateTransition(GameScreen::SCREEN_RACE_START_3);
    }
}

void SceneGame::initLevelCompleteMultiplayer() {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->stateTransition(GameScreen::SCREEN_MULTIPLAYER_COMPLETE);
    }
    int background = AppEngine::getLevelData()->getCurrentLevelObject()->getCompletionBackground();
    switch (background) {
    case 6: background = 20; break;
    case 7: background = 21; break;
    case 8: background = 22; break;
    case 9: background = 23; break;
    case 10: background = 24; break;
    case 11: background = 25; break;
    case 12: background = 26; break;
    case 13: background = 27; break;
    case 14: background = 28; break;
    case 15: background = 29; break;
    case 16: background = 30; break;
    case 17: background = 31; break;
    case 18: background = 32; break;
    case 19: background = 33; break;
    }
    m_engine->getQuadManager().loadQuads(background);
    m_engine->startFadeIn(true);
}

void SceneGame::initLevelCompleteSingle() {
    switch (AppEngine::getLevelData()->getGameMode()) {
    case LevelData::GAME_MODE_STORY:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_STORY_END_OF_LEVEL_PROMPT, 0, 0, 0, 0);
        break;
    case LevelData::GAME_MODE_SPEEDRUN:
    case LevelData::GAME_MODE_MULTIPLAYER_RACE:
    case LevelData::GAME_MODE_MULTIPLAYER_TIMED:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_SPEEDRUN_END_OF_LEVEL_PROMPT, 0, 0, 0, 0);
        break;
    case LevelData::GAME_MODE_CHALLENGE:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_CHALLENGE_END_OF_LEVEL_PROMPT, 0, 0, 0, 0);
        break;
    }
    const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
    m_engine->getQuadManager().loadQuads(level->getCompletionBackground());
    m_engine->startFadeIn(true);
}

void SceneGame::deinitMenuPaused() {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->menuResume();
    }
}

void SceneGame::updateMenuPaused(int dt) {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->update(dt);
    }
}

void SceneGame::initMenuPaused() {
    QuadManager& quads = m_engine->getQuadManager();
    quads.setMeshVisible(95, false);
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->menuPause();
    }
    if (m_prevState == STATE_GAME) {
        quads.playAnim(10, 2);
    }
}

void SceneGame::renderGame(midp::Graphics* g) {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        GameScreen* screen = m_gameScreens[i];
        m_map.cullView(*screen->getCameraFrustum());
        m_map.updateObjectCulling(i);
        if (m_gameScreens.m_length == 2) {
            screen->setupLocatorQuadForRender(m_gameScreens[i ^ 1]->getPlayerObject());
        }
#ifdef MEDECOMP_PORT
        MathVector remote;
        if (m_gameScreens.m_length == 1 && port::net::raceOpponent()) {
            bool here = port::net::remoteLocator(&remote);
            m_engine->getQuadManager().setMeshVisible(72, here);
            if (here) {
                screen->setupLocatorQuad(remote);
            }
        }
#endif
        screen->render(g);
    }
}

void SceneGame::renderAbout(midp::Graphics* g) {
    renderGame(g);
}

void SceneGame::renderHelp(midp::Graphics* g) {
    renderGame(g);
    m_engine->renderBgFade(g);
}

void SceneGame::renderGameOptions(midp::Graphics* g) {
    renderGame(g);
    m_engine->renderBgFade(g);
}

void SceneGame::renderSoundOptions(midp::Graphics* g) {
    renderGame(g);
    m_engine->renderBgFade(g);
}

void SceneGame::updateAccelerometer(int dt) {
    midp::AccelerometerIPhone* accel = midp::AccelerometerIPhone::getStaticAccelerometer();
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    accel->getRawXYZ(&x, &y, &z);
    m_tiltFilter.setTargetValue(atan2f(y, sqrtf(x * x + z * z)) * (-1.0 / 1.570796f));
    float prev = m_tiltFilter.getFilteredValue();
    m_tiltFilter.update(dt);
    if (dt > 0) {
        m_tiltVelocity = (m_tiltFilter.getFilteredValue() - prev) * 1000.0f / dt;
    }
}

void SceneGame::updateSound(int dt) {
    m_soundSequencer.update(dt);
    m_engine->getSoundManager().setGroupPitch(6, getGameTimeFactor());
}

void SceneGame::initGame() {
    m_gameActive = true;
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->getGameCamera()->setClipping(3.5f, 250.0f);
    }
    m_achievementData->registerLevelStart(AppEngine::getLevelData()->getCurrentLevelIndex());
}

void SceneGame::initState() {
    switch (m_state) {
    case STATE_INTRO_CAM:
        for (int i = 0; i != m_gameScreens.m_length; i++) {
            m_gameScreens[i]->getGameCamera()->setClipping(1.0f, 250.0f);
        }
        break;
    case STATE_RACE_START:
        initRaceStart();
        break;
    case STATE_GAME:
        initGame();
        break;
    case STATE_FADE_OUT:
    case STATE_EXIT_FADE:
        m_engine->startFadeOut(true, 0xffffff);
        break;
    case STATE_LEVEL_COMPLETE_SINGLE:
        initLevelCompleteSingle();
        break;
    case STATE_LEVEL_COMPLETE_MULTIPLAYER:
        initLevelCompleteMultiplayer();
        break;
    case STATE_MENU_PAUSED:
        initMenuPaused();
        break;
    case STATE_GAME_OPTIONS:
        initGameOptions();
        break;
    case STATE_SOUND_OPTIONS:
        initSoundOptions();
        break;
    case STATE_HELP:
        initHelp();
        break;
    case STATE_ABOUT:
        initAbout();
        break;
    case STATE_FINISHED_GAME_STORY:
        initFinishedGameStory();
        break;
    case STATE_MEDIA_PICKER:
        initMediaPicker();
        break;
    case STATE_UPLOAD_CONFIRM:
#ifdef MEDECOMP_PORT
        if (m_newRecord && medecomp_player_level_no_board()) {
            m_engine->startFadeOut(true, 0xffffff);
            break;
        }
#endif
        if (m_newRecord) {
            m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_UPLOAD_CONFIRMATION, 0, 0, 0, 0);
            const Level* level = AppEngine::getLevelData()->getCurrentLevelObject();
            m_engine->getQuadManager().loadQuads(level->getCompletionBackground());
        } else {
            m_engine->startFadeOut(true, 0xffffff);
        }
        break;
    case STATE_DISPLAY_NAME_ENTRY:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_DISPLAY_NAME_ENTRY, 0, 0, 0, 0);
        break;
    case STATE_MAYHEM_WAIT:
        initMayhemWait();
        break;
    case STATE_CHALLENGE_USER:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_CHALLENGE_USER, 0, 0, 0, 0);
        break;
    case STATE_CHALLENGE_COMMENT:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_CHALLENGE_COMMENT_ENTRY, 0, 0, 0, 0);
        break;
    default:
        break;
    }
}

void SceneGame::renderLoadingFirstPass(midp::Graphics* g) {
    if (m_world == NULL) {
        return;
    }
    m3g::Graphics3D* g3d = m_engine->getGraphics3D();
    g3d->bindTarget(g);
    g3d->render(m_world);
    g3d->releaseTarget();
}

void SceneGame::processUploadQueue() {
    std::map<m3g::Image2D*, m3g::Image2D*>::iterator it = m_uploadQueue.begin();
    if (it != m_uploadQueue.end()) {
        it->second->getWidth();
        it->second->getHeight();
        it->second->uploadImage();
        m_uploadQueue.erase(it);
        it = m_uploadQueue.begin();
    }
}

void SceneGame::extractUploadQueueScoped(m3g::Node* node, int scope) {
    if (node == NULL) {
        return;
    }
    m3g::Mesh* mesh = m3g::m3g_cast<m3g::Mesh>(node);
    if (mesh != NULL && (mesh->getScope() & scope) != 0) {
        int count = mesh->getSubmeshCount();
        for (int i = 0; i < count; i++) {
            m3g::Appearance* appearance = mesh->getAppearance(i);
            if (appearance == NULL) {
                continue;
            }
            m3g::Texture2D* texture0 = appearance->getTexture(0);
            m3g::Texture2D* texture1 = appearance->getTexture(1);
            if (texture0 != NULL) {
                m_uploadQueue[texture0->getImage()] = texture0->getImage();
            }
            if (texture1 != NULL) {
                m_uploadQueue[texture1->getImage()] = texture1->getImage();
            }
        }
        return;
    }
    m3g::Group* group = m3g::m3g_cast<m3g::Group>(node);
    if (group != NULL) {
        int count = group->getChildCount();
        for (int i = 0; i < count; i++) {
            extractUploadQueueScoped(group->getChild(i), scope);
        }
    }
}

void SceneGame::extractUploadQueue(m3g::Node* node) {
    extractUploadQueueScoped(node, 1);
}

void SceneGame::renderLoading(midp::Graphics* g) {
    switch (m_loadingStep) {
    case LOADING_COLLECT_TEXTURES: {
        QuadManager& quads = m_engine->getQuadManager();
        extractUploadQueue(m_world);
        extractUploadQueueScoped(quads.getWorld(), 0x10);
        extractUploadQueueScoped(quads.getWorld(), 0x20);
        m_loadingStep++;
        break;
    }
    case LOADING_WARM_SHADERS: {
        m3g::Transform identity;
        if (m3g::g_usingGLES20) {
            AppEngine::getCanvas()->getReflectionShader()->bind(identity);
            m3g::OpenGLES20ShaderSupport::getInstance()->warmCache();
        }
        m_loadingStep++;
        break;
    }
    case 9:
    case 10:
        m_loadingStep++;
        break;
    case LOADING_RENDER_QUADS: {
        QuadManager& quads = m_engine->getQuadManager();
        m3g::Graphics3D* g3d = m_engine->getGraphics3D();
        g3d->bindTarget(g);
        quads.render(g3d, 0x30);
        g3d->releaseTarget();
        m_loadingStep++;
        break;
    }
    case LOADING_UPLOAD_TEXTURES:
        processUploadQueue();
        if (uploadQueueExhausted()) {
            renderLoadingFirstPass(g);
            m_loadingStep++;
        }
        break;
    }
    m_engine->renderLoading(g);
    TextManager* text = m_engine->getTextManager();
    text->drawString(g, m_loadingProgress == 100 ? TEXT_TAP_TO_CONTINUE : TEXT_LOADING, 7,
                     m_engine->getWidth() >> 1, m_engine->getHeight() - 20, 0x22);
}

void SceneGame::deinitState() {
    switch (m_state) {
    case STATE_MENU_PAUSED:
        deinitMenuPaused();
        break;
    case STATE_GAME_OPTIONS:
    case STATE_SOUND_OPTIONS:
        AppEngine::getCanvas()->saveRMSAppSettings();
        break;
    case STATE_FINISHED_GAME_STORY:
        deinitFinishedGameStory();
        break;
    default:
        break;
    }
}

void SceneGame::stateTransition(GameState state) {
    if (m_state == state || m_state == STATE_FADE_OUT) {
        return;
    }
    if (m_pausedState == STATE_NONE && state == STATE_MENU_PAUSED) {
        m_pausedState = m_state;
    }
    m_prevState = m_state;
    deinitState();
    m_state = state;
    initState();
    m_stateTime = 0;
}

void SceneGame::updateMayhemWait(int dt) {
    if (m_mayhemResetSession) {
        if (m_engine->m_mayhemToken == NULL ||
            m_engine->m_mayhemToken->GetResult() != Mayhem::Request::RESULT_PENDING) {
            m_engine->m_mayhemId = "";
            m_engine->m_mayhemToken = NULL;
            m_mayhemPendingStep = MAYHEM_POST_STAT;
            if (!m_engine->IsMayhemSessionValid()) {
                m_userRequest = new Mayhem::GetUserRequest(true);
                m_mayhemStep = MAYHEM_GET_USER;
            } else {
                if (!uploadSpeedrunStat(false)) {
                    m_pleaseWaitWindow->Succeeded(TEXT_TIME_TOO_LONG);
                    m_mayhemStep = MAYHEM_DONE;
                    m_engine->getWindowStore().getNetworkWaitEffect()->stop();
                    return;
                }
            }
            m_mayhemResetSession = false;
        }
        return;
    }

    switch (m_mayhemStep) {
    case MAYHEM_FB_LOGIN: {
        FBConnection* fb = NULL;
        int login = fb->GetLoginResult();
        if (login == FBConnection::LOGIN_IN_PROGRESS) {
            break;
        }
        if (login == FBConnection::LOGIN_SUCCEEDED) {
            m_userRequest = new Mayhem::GetUserRequest(fb->m_userId, true);
            m_mayhemStep = MAYHEM_GET_USER;
            break;
        }
        m_pleaseWaitWindow->Failed(TEXT_UPLOAD_ERROR);
        m_engine->getWindowStore().getNetworkWaitEffect()->stop();
        m_mayhemStep = MAYHEM_DONE;
        break;
    }

    case MAYHEM_TOKEN: {
        const midp::SafeReferenceCountedPointer<Mayhem::Token> token(m_engine->m_mayhemToken);
        if (token->GetResult() == Mayhem::Request::RESULT_PENDING) {
            break;
        }
        if (!token->IsValid()) {
            m_pleaseWaitWindow->Failed(TEXT_UPLOAD_ERROR);
            m_engine->getWindowStore().getNetworkWaitEffect()->stop();
            m_mayhemStep = MAYHEM_DONE;
            break;
        }
        m_mayhemStep = m_mayhemPendingStep;
        if (m_mayhemStep == MAYHEM_POST_CHALLENGE) {
            uploadChallenge();
        } else if (m_mayhemStep == MAYHEM_POST_STAT) {
            if (!uploadSpeedrunStat(false)) {
                m_pleaseWaitWindow->Succeeded(TEXT_TIME_TOO_LONG);
                m_mayhemStep = MAYHEM_DONE;
                m_engine->getWindowStore().getNetworkWaitEffect()->stop();
                break;
            }
        }
        m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, "");
        break;
    }

    case MAYHEM_GET_USER:
        if (m_userRequest->GetResult() == Mayhem::Request::RESULT_PENDING) {
            break;
        }
        if (!m_userRequest->IsValid()) {
            m_engine->getWindowStore().getNetworkWaitEffect()->stop();
            m_pleaseWaitWindow->close(UI::RESULT_NONE);
            stateTransition(STATE_DISPLAY_NAME_ENTRY);
            break;
        }
        m_engine->m_mayhemId = m_userRequest->GetMayhemID();
        if (m_engine->m_mayhemToken == NULL) {
            m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, "");
        } else if (m_engine->m_mayhemToken->IsValid()) {
            if (uploadSpeedrunStat(false)) {
                m_mayhemStep = MAYHEM_POST_STAT;
            } else {
                m_pleaseWaitWindow->Succeeded(TEXT_TIME_TOO_LONG);
                m_mayhemStep = MAYHEM_DONE;
                m_engine->getWindowStore().getNetworkWaitEffect()->stop();
            }
            break;
        } else {
            m_engine->m_mayhemToken->Refresh();
        }
        m_mayhemStep = MAYHEM_TOKEN;
        break;

    case MAYHEM_POST_USER:
        if (m_userRequest->GetResult() == Mayhem::Request::RESULT_PENDING) {
            break;
        }
        if (m_userRequest->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
            m_engine->m_mayhemId = m_userRequest->GetMayhemID();
            m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, "");
            m_mayhemStep = MAYHEM_TOKEN;
            break;
        }
        m_pleaseWaitWindow->Failed(TEXT_UPLOAD_ERROR);
        m_engine->getWindowStore().getNetworkWaitEffect()->stop();
        m_mayhemStep = MAYHEM_DONE;
        break;

    case MAYHEM_POST_STAT:
        if (m_postStatRequest->GetResult() != Mayhem::Request::RESULT_PENDING) {
            std::string errorType = m_postStatRequest->GetErrorType();
            std::string notHigher("POSTED_SCORE_IS_NOT_HIGHER");
            if (errorType == notHigher) {
                m_pleaseWaitWindow->Succeeded(TEXT_SCORE_NOT_HIGHER);
            } else if (m_postStatRequest->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
                m_pleaseWaitWindow->Succeeded(TEXT_SCORE_UPLOADED);
            } else {
                m_pleaseWaitWindow->Failed(TEXT_UPLOAD_ERROR);
            }
            m_engine->getWindowStore().getNetworkWaitEffect()->stop();
            m_mayhemStep = MAYHEM_DONE;
        }
        break;

    case MAYHEM_POST_CHALLENGE:
        if (m_postChallenge->GetResult() != Mayhem::Request::RESULT_PENDING) {
            if (m_postChallenge->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
                m_pleaseWaitWindow->Succeeded(TEXT_CHALLENGE_UPLOADED);
            } else {
                m_pleaseWaitWindow->Failed(TEXT_CHALLENGE_ERROR);
            }
            m_engine->getWindowStore().getNetworkWaitEffect()->stop();
            m_exitState = STATE_LEVEL_COMPLETE_SINGLE;
            m_mayhemStep = MAYHEM_CLOSE;
        }
        break;

    case MAYHEM_CLOSE:
        if (m_engine->getWindowStore().getWindowResult() != UI::RESULT_NONE) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(m_exitState);
            deinitState();
        }
        break;

    case MAYHEM_DONE: {
        if (m_engine->isFadedOut()) {
            if (m_pleaseWaitWindow != 0) {
                m_pleaseWaitWindow->close(UI::RESULT_NONE);
            }
            stateTransition(m_exitState);
        }
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_NONE) {
            break;
        }
        m_engine->getWindowStore().clearWindowResult();
        switch (result) {
        case UI::PleaseWaitWindow::BUTTON_RETRY:
            m_mayhemStep = MAYHEM_POST_STAT;
            m_state = STATE_NONE;
            stateTransition(STATE_MAYHEM_WAIT);
            break;
        case UI::PleaseWaitWindow::BUTTON_BACK:
            stateTransition(m_gameScreens.m_length == 1 ? STATE_LEVEL_COMPLETE_SINGLE
                                                        : STATE_LEVEL_COMPLETE_MULTIPLAYER);
            break;
        case UI::PleaseWaitWindow::BUTTON_OK:
            m_engine->startFadeOut(true, 0xffffff);
            break;
        }
        break;
    }
    }
}

void SceneGame::updateMediaPicker() {
    if (midp::Display::getDisplay(m_engine->getMIDlet())->getCurrent() != m_engine) {
        return;
    }
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->resume();
    }
    stateTransition(STATE_MENU_PAUSED);
}

void SceneGame::triggerPopupBox(int text) {
    UI::PopupBox* popup = new UI::PopupBox(text);
    m_engine->getWindowStore().pushWindow(popup);
    popup->setDeleteOnClosed(true);
    m_popupReturnState = m_state;
    stateTransition(STATE_POPUP_BOX);
}

void SceneGame::resetCheckpoint() {
    m_map.resetCheckpoint();
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        GameScreen* screen = m_gameScreens[i];
        screen->getGameCamera()->startSmoothTracking(screen->getPlayerObject(), true);
    }
    stateTransition(STATE_GAME);
    if (m_ghost != NULL) {
        m_map.addObject(m_ghost);
    }
    clearGameTimeFactor();
    m_combatTime = 0;
}

void SceneGame::updateAbout(int dt) {
    if (m_engine->getWindowStore().getWindowResult() != 0) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_MENU_PAUSED);
    }
}

void SceneGame::updateHelp(int dt) {
    if (m_engine->getWindowStore().getWindowResult() != 0) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_MENU_PAUSED);
    }
}

void SceneGame::updatePopupBox(int dt) {
    if (m_engine->getWindowStore().getWindowResult() != 0) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(m_popupReturnState);
    }
}

void SceneGame::updateGameOptions(int dt) {
    if (m_engine->getWindowStore().getWindowResult() != 0) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_MENU_PAUSED);
        m_engine->saveGameOptions();
    }
}

void SceneGame::updateSoundOptions(int dt) {
    if (m_engine->getWindowStore().getWindowResult() != 0) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_MENU_PAUSED);
    }
    SoundManager& sound = m_engine->getSoundManager();
    UI::SoundOptions* options = m_soundOptionsWindow;
    float music = options->getMusicLevel();
    float effects = options->getEffectsLevel();
    if (effects != sound.getVolumeSFX()) {
        sound.setVolumeSFX(effects);
        sound.playEvent(SOUND_GO, 1.0f);
    } else if (music != sound.getVolumeMusic()) {
        sound.setVolumeMusic(music);
    }
}

void SceneGame::resumeMenuPaused() {
    SpywareManager::getInstance()->trackResumeLevel(AppEngine::getLevelData()->getCurrentLevelIndex());
    stateTransition(m_pausedState);
    m_pausedState = STATE_NONE;
}

void SceneGame::stateTransitionFade(GameState state) {
    if (m_state == STATE_FADE_OUT) {
        return;
    }
    m_fadeState = state;
    stateTransition(STATE_FADE_OUT);
}

void SceneGame::completeLevel() {
    LevelData* levelData = AppEngine::getLevelData();
    m_map.updateCollectableList();
    m_achievementData->registerLevelComplete();
    bool multiplayer = levelData->isMultiplayerMode();
    SpywareManager* spyware = SpywareManager::getInstance();
    if (multiplayer) {
        spyware->trackLevelFinished(AppEngine::getLevelData()->getCurrentLevelIndex());
    } else {
        spyware->trackMultiplayerFinished();
    }

    const Level* level = levelData->getCurrentLevelObject();
    if (!level->isLevelComplete()) {
        m_firstCompletion = true;
    }
    m_completedRaceTime = m_raceTime;
    m_prevBestTime = level->getBestSpeedRunTimeMillis();
    bool newRecord = AppEngine::getLevelData()->setLevelComplete(m_raceTime, m_map.getCollectableList());

    int numScreens = m_gameScreens.m_length;
    bool multiplayerResults = numScreens != 1;
#ifdef MEDECOMP_PORT
    if (port::net::rivalsNetwork()) {
        multiplayerResults = true;
    }
    if (medecomp_solo_bags()) {
        multiplayerResults = true;
        medecomp_solo_bags_result(m_gameScreens[0]->getPlayerObject()->getNumCollectablesCollected(),
                                  m_map.getNumCollectablesTotal(), m_raceTime);
    }
#endif
    if (multiplayerResults) {
        stateTransitionFade(STATE_LEVEL_COMPLETE_MULTIPLAYER);
        return;
    }
    GhostAnimationRecorder* recorder = m_gameScreens[0]->getPlayerObject()->getGhostAnimation();
    if (recorder != NULL) {
        recorder->write(new midp::String(LAST_GHOST_ANIMATION_FILENAME));
    }
    if (newRecord && numScreens == 1) {
        m_newRecord = true;
        m_ghost = NULL;
        if (recorder != NULL) {
            recorder->write(NULL);
        }
    }
    stateTransitionFade(STATE_LEVEL_COMPLETE_SINGLE);
}

void SceneGame::updateRaceTime(int dt) {
    int mode = AppEngine::getLevelData()->getGameMode();
    int prevTime = m_raceTime;
    if (mode != LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        m_raceTime = (int)(prevTime + dt * getGameTimeFactor());
        return;
    }

    m_raceTime = (int)(prevTime - dt * getGameTimeFactor());
#ifdef MEDECOMP_PORT
    port::net::rivalsSyncTime(&m_raceTime);
#endif
    if (m_raceTime < 10000 && m_raceTime / 1000 != prevTime / 1000) {
        m_engine->getSoundManager().playEvent(SOUND_COUNTDOWN, 1.0f);
    }
    if (m_raceTime <= 0) {
        completeLevel();
        m_engine->getSoundManager().playEvent(SOUND_TIME_OVER, 1.0f);
    }
    if (!AppEngine::getLevelData()->isMultiplayerCombatAllowed()) {
        int collected = 0;
        int numScreens = m_gameScreens.m_length;
        for (int i = 0; i != numScreens; i++) {
            collected += m_gameScreens[i]->getPlayerObject()->getNumCollectablesCollected();
        }
#ifdef MEDECOMP_PORT
        collected += port::net::rivalsRemoteBags();
#endif
        if (m_map.getNumCollectablesTotal() == collected) {
            completeLevel();
            m_engine->getSoundManager().playEvent(SOUND_TIME_OVER, 1.0f);
        }
    }
}

void SceneGame::render(midp::Graphics* g) {
    switch (m_state) {
    case STATE_LOADING:
        renderLoading(g);
        break;
    case STATE_INTRO_CAM:
    case STATE_RACE_START:
    case STATE_GAME:
    case STATE_FADE_OUT:
    case STATE_CAMERA_ANIM:
        renderGame(g);
        renderTopMessage(g);
#ifdef MEDECOMP_PORT
        port::net::renderRace(g);
#endif
        break;
    case STATE_LEVEL_COMPLETE_MULTIPLAYER:
    case STATE_MENU_PAUSED:
    case STATE_POPUP_BOX:
        renderGame(g);
        break;
    case STATE_GAME_OPTIONS:
        renderGameOptions(g);
        break;
    case STATE_SOUND_OPTIONS:
        renderSoundOptions(g);
        break;
    case STATE_HELP:
        renderHelp(g);
        break;
    case STATE_ABOUT:
        renderAbout(g);
        break;
    case STATE_FINISHED_GAME_STORY:
        renderFinishedGameStory(g);
        break;
    case STATE_LEVEL_COMPLETE_SINGLE:
    case STATE_UPLOAD_CONFIRM:
    case STATE_DISPLAY_NAME_ENTRY:
    case STATE_MAYHEM_WAIT:
    case STATE_CHALLENGE_USER:
    case STATE_CHALLENGE_COMMENT:
        m_engine->getQuadManager().setGroupVisible(0, true);
        m_engine->getQuadManager().render(g, 2);
        m_engine->getQuadManager().setGroupVisible(0, false);
        break;
    default:
        break;
    }
}

void SceneGame::updateCamera(int dt) {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->updateCamera(dt);
    }
}

void SceneGame::updateGame(int dt) {
    m_sfxVolume.setTargetValue(m_engine->getSoundManager().getVolumeSFX());
    QuadManager& quads = m_engine->getQuadManager();
    m_gameTimeFactor.update(dt);
    int gameDt = (int)(dt * getGameTimeFactor());
    updateSound(gameDt);
    m_achievementData->update(gameDt);
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->updateEffectAnims(quads, dt);
    }
    quads.updateAnim(1, dt);
    updateAccelerometer(dt);
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        GameScreen* screen = m_gameScreens[i];
        screen->updatePointers(dt);
        screen->applyGestures();
    }
    updateCamera(dt);
    m_map.updateObjects(gameDt);
    m_map.updateObjectCollisions();
    m_map.updateChunkVisibility(gameDt);
    if (m_state == STATE_LEVEL_COMPLETE_SINGLE) {
        return;
    }
    if (m_combatTime > 0) {
        m_combatTime -= dt;
        bool slow = m_combatTime > 0;
#ifdef MEDECOMP_PORT
        slow = slow && !port::net::raceOpponent();
#endif
        setGameTimeFactor(slow ? 0.3f : 1.0f);
    }
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->update(dt);
    }
}

void SceneGame::resetLevel(GameState next) {
    LevelData* levelData = AppEngine::getLevelData();
    const Level* level = levelData->getCurrentLevelObject();
    m_sfxVolume.setSteadyState(m_engine->getSoundManager().getVolumeSFX());
    m_engine->getBGMusic()->playMusic(level->getStartMusicId(), 2);
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->resetLevel();
    }
    m_map.resetLevel();
    m_unk1c8 = 0;
    if (levelData->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
        m_raceTime = levelData->getLevelTimeLimitMillis();
    } else {
        m_raceTime = 0;
    }
    stateTransition(next);
    m_engine->startFadeIn(true);
    startIntroCam();

    int mode = levelData->getGameMode();
    if (mode == LevelData::GAME_MODE_STORY) {
        startTopMessage(level->getObjective(), level->getDateTime());
    } else if (mode == LevelData::GAME_MODE_SPEEDRUN && level->isLevelComplete()
#ifdef MEDECOMP_PORT
               && !port::net::raceOpponent()
#endif
    ) {
        if (m_ghost == NULL) {
            GhostAnimationPlayback* playback = GhostAnimationPlayback::createForCurrentLevel();
            if (playback != NULL) {
                m_ghost = new GameObjectGhost(&m_map, playback);
            }
        } else {
            m_ghost->resetLevel();
        }
        if (m_ghost != NULL) {
            m_map.addObject(m_ghost);
        }
    } else if (levelData->getGameMode() == LevelData::GAME_MODE_CHALLENGE) {
        if (m_ghost == NULL) {
            GhostAnimationPlayback* playback = GhostAnimationPlayback::createFromChallenge();
            if (playback != NULL) {
                m_ghost = new GameObjectGhost(&m_map, playback);
            }
        } else {
            m_ghost->resetLevel();
        }
        if (m_ghost != NULL) {
            m_map.addObject(m_ghost);
        }
    }
    clearGameTimeFactor();
    m_combatTime = 0;
    updateGame(0);
    m_pausedState = STATE_NONE;
}

void SceneGame::userRestartingLevel() {
    SpywareManager::getInstance()->trackRestartLevel(AppEngine::getLevelData()->getCurrentLevelIndex());
    resetLevel(STATE_INTRO_CAM);
}

void SceneGame::updateRaceStart(int dt) {
    updateGame(dt);
    if (!m_gameScreens[0]->isInRaceStart()) {
        stateTransition(STATE_GAME);
    }
}

void SceneGame::pause() {
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->suspend();
    }
    switch (m_state) {
    case STATE_INTRO_CAM:
    case STATE_GAME:
    case STATE_CAMERA_ANIM:
    case STATE_NEXT_LEVEL:
        m_engine->setFadeColor(0xffffff);
        m_engine->startFadeIn(true);
        stateTransition(STATE_MENU_PAUSED);
        m_sfxVolume.setSteadyState(0.0f);
        m_engine->getSoundManager().setVolumeGroup(2, 0.0f);
        m_engine->getSoundManager().setVolumeGroup(6, 0.0f);
        break;
    default:
        break;
    }
}

void SceneGame::unloadSounds() {
    SoundManager& sound = m_engine->getSoundManager();
    sound.unloadEvent(187);
    sound.unloadEvent(79);
    sound.unloadEvent(71);
    sound.unloadEvent(72);
    sound.unloadEvent(73);
    sound.unloadEvent(74);
    sound.unloadEvent(75);
    sound.unloadEvent(76);
    sound.unloadEvent(77);
    sound.unloadEvent(85);
    sound.unloadEvent(86);
    sound.unloadEvent(87);
    sound.unloadEvent(88);
    sound.unloadEvent(89);
    sound.unloadEvent(81);
    sound.unloadEvent(82);
    sound.unloadEvent(83);
    sound.unloadEvent(84);
    sound.unloadEvent(90);
    sound.unloadEvent(91);
    sound.unloadEvent(92);
    sound.unloadEvent(93);
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED &&
        sound.isEventLoaded(SOUND_TIME_OVER)) {
        sound.unloadEvent(SOUND_TIME_OVER);
    }
}

void SceneGame::loadSounds() {
    SoundManager& sound = m_engine->getSoundManager();
    sound.loadEvent(187);
    sound.loadEvent(79);
    sound.loadEvent(71);
    sound.loadEvent(72);
    sound.loadEvent(73);
    sound.loadEvent(74);
    sound.loadEvent(75);
    sound.loadEvent(76);
    sound.loadEvent(77);
    sound.loadEvent(85);
    sound.loadEvent(86);
    sound.loadEvent(87);
    sound.loadEvent(88);
    sound.loadEvent(89);
    sound.loadEvent(81);
    sound.loadEvent(82);
    sound.loadEvent(83);
    sound.loadEvent(84);
    sound.loadEvent(90);
    sound.loadEvent(91);
    sound.loadEvent(92);
    sound.loadEvent(93);
    if (AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED &&
        !sound.isEventLoaded(SOUND_TIME_OVER)) {
        sound.loadEvent(SOUND_TIME_OVER);
    }
}

void SceneGame::updateLoadingState(int dt) {
    switch (m_loadingStep) {
    case 0:
        break;
    case 1: {
        QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
        quads.loadQuads(3);
        int mode = AppEngine::getLevelData()->getGameMode();
        if (mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED || mode == LevelData::GAME_MODE_MULTIPLAYER_RACE) {
            quads.loadQuads(5);
            quads.setMeshVisible(72, true);
#ifdef MEDECOMP_PORT
            if (port::net::rivalsNetwork() || medecomp_solo_bags()) {
                quads.setMeshVisible(72, false);
            }
#endif
        } else if (mode == LevelData::GAME_MODE_SPEEDRUN || mode == LevelData::GAME_MODE_CHALLENGE) {
#ifdef MEDECOMP_PORT
            if (port::net::raceOpponent()) {
                quads.loadQuads(5);
                quads.setMeshVisible(72, false);
            }
            quads.setGroupVisible(46, !medecomp_player_level_playing());
#else
            quads.setGroupVisible(46, true);
#endif
        }
        if (AppEngine::getLevelData()->isMultiplayerMode()) {
            quads.loadQuads(5);
        }
        quads.setAnimFrame(1, 3);
        break;
    }
    case 2: {
        AppEngine::getAnimationBlenderData()->loadData();
        if (m_world == NULL) {
            m_world = new m3g::World();
            midp::INCREF(m_world);
            bool split = AppEngine::getLevelData()->isMultiplayerMode();
#ifdef MEDECOMP_PORT
            if (port::net::rivalsNetwork() || medecomp_solo_bags()) {
                split = false;
            }
#endif
            if (split) {
                midp::array<midp::ReferenceCountedPointer<GameScreen> > screens(2);
                screens[0] = new GameScreen(this, GameScreen::TYPE_SPLIT_FIRST, m_world);
                screens[1] = new GameScreen(this, GameScreen::TYPE_SPLIT_SECOND, m_world);
                m_gameScreens = screens;
            } else {
                midp::array<midp::ReferenceCountedPointer<GameScreen> > screens(1);
                screens[0] = new GameScreen(this, GameScreen::TYPE_SINGLE, m_world);
                m_gameScreens = screens;
            }
        }
        m3g::Node* model = AppEngine::getM3GAssets()->loadModel(21, 4);
        midp::INCREF(model);
        midp::DECREF(model);
        m3g::AppearanceBase* appearance = AppEngine::getM3GAssets()->loadTexturedAppearance(115, 4);
        midp::INCREF(appearance);
        midp::DECREF(appearance);
        break;
    }
    case 3:
        AppEngine::getGameObjectData()->loadData();
        AppEngine::getGameObjectRunnerData()->loadData();
        break;
    case 4: {
        LevelData* levelData = AppEngine::getLevelData();
        m_map.loadMap(levelData->getLevel(levelData->getCurrentLevelIndex())->getMapResId(), m_world);
        if (m_gameScreens.m_length == 2) {
            GameObjectPlayer* first = m_gameScreens[0]->getPlayerObject();
            GameObjectPlayer* second = m_gameScreens[1]->getPlayerObject();
            first->setZOffset(0.25f);
            second->setZOffset(-0.25f);
        }
        if (m3g::g_usingGLES20) {
            int reflectionId = m_map.getModelSet()->getUserId(0);
            if (reflectionId != -1) {
                m3g::Image2D* reflection =
                    AppEngine::getCanvas()->getResourceManager().loadM3GImage2D(reflectionId);
                m_engine->getReflectionShader()->setReflectionMap(reflection);
                midp::DECREF(reflection);
            }
        }
        m3g::Background* background = new m3g::Background();
        background->setColorClearEnable(true);
        background->setColor(m_map.getModelSet()->getBackgroundFillColor());
        background->setDepthClearEnable(true);
        m_world->setBackground(background);
        break;
    }
    case 5:
        loadSounds();
        m_soundSequencer.loadData();
        break;
    case 6:
        AppEngine::getM3GAssets()->freeCaches(8);
        break;
    case LOADING_DONE:
        m_loadingProgress = 100;
        m_loadingThreadState = 2;
        m_loadingThread = NULL;
        return;
    default:
        return;
    }
    m_loadingStep++;
}

void SceneGame::updateLoading(int dt) {
    m_engine->updateLoading(dt);
    if (m_loadingProgress == 100) {
        return;
    }
    if (m_loadingThread == NULL) {
        if (m_engine->isFading()) {
            return;
        }
        m_loadingThread = new midp::Thread(this);
        m_loadingThreadState = 0;
        m_loadingThread->start();
    } else {
        midp::Thread::sleep(40);
    }
}

void SceneGame::update(int dt) {
    m_sfxVolume.setTargetValue(0.0f);
    m_stateTime += dt;
    if (m_storyStartTimer > 0) {
        m_storyStartTimer -= dt;
    }

    switch (m_state) {
    case STATE_LOADING:
#ifdef MEDECOMP_PORT
        if (medecomp_player_level_test() && m_loadingProgress == 100 && !m_engine->isFading() &&
            !m_engine->isFadedOut()) {
            m_engine->startFadeOut(true, 0xffffff);
        }
#endif
        if (m_engine->isFadedOut() && m_loadingProgress == 100) {
            resetLevel(STATE_INTRO_CAM);
            m_state = m_stateAfterLoading;
            for (int i = 0; i != m_gameScreens.m_length; i++) {
                m_gameScreens[i]->updateCamera(0);
            }
            m_engine->deinitLoadingScreen();
        } else {
            updateLoading(dt);
        }
        break;
    case STATE_INTRO_CAM:
        updateGame(dt);
        if (!m_gameScreens[0]->getGameCamera()->isAnimating()) {
#ifdef MEDECOMP_PORT
            if (port::net::raceHoldStart()) {
                break;
            }
#endif
            if (AppEngine::getLevelData()->isRaceMode() || AppEngine::getLevelData()->isMultiplayerMode()) {
                stateTransition(STATE_RACE_START);
            } else {
                stateTransition(STATE_GAME);
                m_storyStartTimer = 2000;
            }
        }
        break;
    case STATE_RACE_START:
        updateRaceStart(dt);
        break;
    case STATE_GAME:
        updateRaceTime(dt);
        updateGame(dt);
        break;
    case STATE_FADE_OUT:
        updateGame(dt);
        if (m_engine->isFadedOut()) {
            m_state = STATE_NONE;
            stateTransition(m_fadeState);
        }
        break;
    case STATE_MENU_PAUSED:
        updateMenuPaused(dt);
        break;
    case STATE_GAME_OPTIONS:
        updateGameOptions(dt);
        break;
    case STATE_SOUND_OPTIONS:
        updateSoundOptions(dt);
        break;
    case STATE_POPUP_BOX:
        updatePopupBox(dt);
        break;
    case STATE_CAMERA_ANIM:
        updateCamera(dt);
        if (!m_gameScreens[0]->getGameCamera()->isAnimating()) {
            stateTransition(STATE_GAME);
        }
        break;
    case STATE_HELP:
        updateHelp(dt);
        break;
    case STATE_ABOUT:
        updateAbout(dt);
        break;
    case STATE_RESTART_LEVEL:
        if (m_engine->isFadedOut()) {
            resetLevel(STATE_INTRO_CAM);
        }
        break;
    case STATE_NEXT_LEVEL:
#ifdef MEDECOMP_PORT
        if (port::net::raceHostChooses()) {
            port::net::raceChooseNext();
            m_gameActive = false;
            m_engine->changeScene(2, 6);
            break;
        }
#endif
        if (AppEngine::getLevelData()->isCurrentLevelLastStory()) {
            stateTransition(STATE_FINISHED_GAME_STORY);
            m_engine->startFadeIn(true);
        } else {
            AppEngine::getLevelData()->nextStoryLevel();
            m_gameActive = false;
            m_engine->changeScene(3);
        }
        break;
    case STATE_EXIT_LEVEL:
        if (m_engine->isFadedOut()) {
            m_gameActive = false;
            m_engine->changeScene(2, 21);
        }
        break;
    case STATE_FINISHED_GAME_STORY:
        updateFinishedGameStory(dt);
        break;
    case STATE_MEDIA_PICKER:
        updateMediaPicker();
        break;
    case STATE_UPLOAD_CONFIRM:
        if (m_engine->isFadedOut()) {
            stateTransition(m_exitState);
        } else {
            int result = m_engine->getWindowStore().getWindowResult();
            if (result != 0) {
                m_engine->getWindowStore().clearWindowResult();
                if (result == WINDOW_RESULT_CANCEL) {
                    m_engine->getWindowStore().clearWindowResult();
                    m_engine->startFadeOut(true, 0xffffff);
                } else {
                    m_mayhemStep = MAYHEM_POST_STAT;
                    stateTransition(STATE_MAYHEM_WAIT);
                }
            }
        }
        break;
    case STATE_DISPLAY_NAME_ENTRY: {
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == WINDOW_RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_UPLOAD_CONFIRM);
        } else if (result == WINDOW_RESULT_OK) {
            m_engine->getWindowStore().clearWindowResult();
            m_userRequest =
                midp::SafeReferenceCountedPointer<Mayhem::UserRequest>(new Mayhem::PostUserRequest(m_displayName));
            m_mayhemStep = MAYHEM_POST_USER;
            stateTransition(STATE_MAYHEM_WAIT);
        }
        break;
    }
    case STATE_MAYHEM_WAIT:
        updateMayhemWait(dt);
        break;
    case STATE_EXIT_FADE:
        if (m_engine->isFadedOut()) {
            stateTransition(m_exitState);
        }
        break;
    case STATE_CHALLENGE_USER: {
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == WINDOW_RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_LEVEL_COMPLETE_SINGLE);
            deinitState();
        } else if (result == WINDOW_RESULT_OK) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_CHALLENGE_COMMENT);
            deinitState();
        }
        break;
    }
    case STATE_CHALLENGE_COMMENT: {
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == WINDOW_RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_CHALLENGE_USER);
            deinitState();
        } else if (result == WINDOW_RESULT_OK) {
            m_engine->getWindowStore().clearWindowResult();
            m_mayhemStep = MAYHEM_POST_CHALLENGE;
            stateTransition(STATE_MAYHEM_WAIT);
            deinitState();
        }
        break;
    }
    default:
        break;
    }

    updateTopMessage(dt);
    m_sfxVolume.update(dt);
    m_engine->getSoundManager().setVolumeGroup(2, midp::Math::min(m_sfxVolume.getFilteredValue(), 1.0f));
    m_engine->getSoundManager().setVolumeGroup(6, midp::Math::min(m_sfxVolume.getFilteredValue(), 1.0f));
}

void SceneGame::run() {
    for (;;) {
        if (m_loadingThreadState == 0) {
            updateLoadingState(0);
        } else if (m_loadingThreadState == 2) {
            return;
        } else {
            midp::Thread::sleep(1000);
        }
    }
}

void SceneGame::resetCheckPointsLevel() {
    for (int i = 0; i != m_gameScreens.m_length; i++) {
        m_gameScreens[i]->setCheckpointObject(NULL);
    }
}

void SceneGame::end() {
    unloadSounds();
    QuadManager& quads = m_engine->getQuadManager();
    quads.freeQuads(3);
    quads.freeQuads(0);
    quads.setGroupVisible(73, false);
    stateTransition(STATE_NONE);
}

void SceneGame::start(int param) {
    m_state = STATE_LOADING;
    LevelData* levelData = AppEngine::getLevelData();
    m_engine->initLoadingScreen(levelData->getCurrentLoadingScreen());
    m_engine->startFadeIn(false);
    m_engine->getBGMusic()->playMusic(2, 2);
    int mode = levelData->getGameMode();
    int level = levelData->getCurrentLevelIndex();
    switch (mode) {
    case LevelData::GAME_MODE_STORY:
        SpywareManager::getInstance()->trackStoryLevelStarted(level);
        break;
    case LevelData::GAME_MODE_SPEEDRUN:
    case LevelData::GAME_MODE_CHALLENGE:
        SpywareManager::getInstance()->trackRaceLevelStarted(level);
        break;
    case LevelData::GAME_MODE_MULTIPLAYER_RACE:
    case LevelData::GAME_MODE_MULTIPLAYER_TIMED:
        SpywareManager::getInstance()->trackMultiplayerStarted();
        break;
    }
}

SceneGame::~SceneGame() {
    m_map.freeMap();
    midp::DECREF(m_world);
    m_world = NULL;
    AppEngine::getM3GAssets()->freeCaches(4);
    m_achievementData->registerLevelEnd();
}

SceneGame::SceneGame(AppEngine* engine)
    : Scene(engine),
      m_world(NULL),
      m_raceTime(0),
      m_unk1c8(5000),
      m_tiltFilter(SignalFilter::ONE_POLE_LP, 300.0f, 0.0f),
      m_tiltVelocity(0),
      m_state((GameState)0),
      m_fadeState((GameState)0),
      m_prevState((GameState)0),
      m_pausedState((GameState)0),
      m_stateTime(0),
      m_gameActive(false),
      m_loadingThread(NULL),
      m_loadingThreadState(0),
      m_loadingStep(0),
      m_stateAfterLoading((GameState)2),
      m_soundEffectModifying(-1),
      m_soundMusicModifying(-1),
      m_soundOptionsWindow(NULL),
      m_sfxVolume(SignalFilter::SMOOTHSTEP_FIR, 800.0f, 0.0f),
      m_popupReturnState(STATE_GAME),
      m_finishedStoryTimer(0),
      m_topMessageText(0x800),
      m_topMessageText2(0x800),
      m_topMessagePhase(0),
      m_soundSequencer(this, engine->getSoundManager(), m_soundPoolManager),
      m_gameTimeFactor(SignalFilter::SMOOTHSTEP_FIR, 500.0f, 1.0f),
      m_combatTime(0),
      m_achievementData(AppEngine::getAchievementData()),
      m_loadingScrollY(0),
      m_loadingScrolled(false),
      m_ghost(NULL),
      m_completedRaceTime(-1),
      m_prevBestTime(-1),
      m_newRecord(false),
      m_firstCompletion(false),
      m_exitState((GameState)0),
      m_pleaseWaitWindow(NULL),
      m_mayhemStep(0),
      m_mayhemPendingStep(0),
      m_userRequest(NULL),
      m_postChallenge(NULL),
      m_postStatRequest(NULL),
      m_storyStartTimer(-1) {
#ifdef MEDECOMP_PORT
    m_mayhemResetSession = false;
#endif
}

int SceneGame::getRaceTime() const {
    return m_raceTime;
}

int SceneGame::numGameScreens() {
    return m_gameScreens.m_length;
}

GameScreen* SceneGame::getGameScreen(int index) {
    return m_gameScreens[index];
}

m3g::Group* SceneGame::getM3GWorld() {
    return m_world;
}

GameObjectPlayer* SceneGame::getSinglePlayerObject() {
    if (m_gameScreens.m_length != 1) {
        return NULL;
    }
    return m_gameScreens[0]->getPlayerObject();
}

int SceneGame::getState() {
    return m_state;
}

SoundSequencer& SceneGame::getSoundSequencer() {
    return m_soundSequencer;
}

SoundEventPoolManager& SceneGame::getSoundPoolManager() {
    return m_soundPoolManager;
}

MEdgeMap& SceneGame::getMap() {
    return m_map;
}

void SceneGame::setExitState(GameState state) {
    m_exitState = state;
}

int SceneGame::getSoundEffectModifying() {
    return m_soundEffectModifying;
}

void SceneGame::setSoundEffectModifying(int value) {
    m_soundEffectModifying = value;
}

void SceneGame::clearSoundEffectModifying() {
    m_soundEffectModifying = -1;
}

int SceneGame::getSoundMusicModifying() {
    return m_soundMusicModifying;
}

void SceneGame::setSoundMusicModifying(int value) {
    m_soundMusicModifying = value;
}

void SceneGame::clearSoundMusicModifying() {
    m_soundMusicModifying = -1;
}
