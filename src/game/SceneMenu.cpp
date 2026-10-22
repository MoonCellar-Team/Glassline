#include "game/SceneMenu.h"

extern "C" int sscanf(const char* s, const char* format, ...);

#include "ea_online/FBConnection.h"
#include "ea_online/Mayhem/Challenge.h"
#include "ea_online/Mayhem/GetUserRequest.h"
#include "ea_online/Mayhem/Token.h"
#include "ea_online/PendingChallenge.h"
#include "ea_online/SpywareManager.h"
#include "ea_online/ea/EASpywareManager.h"
#include "game/AchievementData.h"
#include "game/AnimPlayer.h"
#include "game/AnimPlayer3D.h"
#include "game/AnimationManager.h"
#include "game/AppEngine.h"
#include "game/BGMusic.h"
#include "game/GameCamera.h"
#include "game/GameCommon.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MathTrig.h"
#include "game/MediaPicker.h"
#include "game/MenuMainSubMenu.h"
#include "game/QuadManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "game/UI/AboutMenu.h"
#include "game/UI/AchievementWindow.h"
#include "game/UI/NetworkWaitEffect.h"
#include "game/UI/PendingChallengeWindow.h"
#ifdef MEDECOMP_PORT
#include "game/UI/PleaseWaitWindow.h"
#include "port/net.h"
#endif
#include "game/UI/SoundOptions.h"
#include "game/UI/WindowStore.h"
#include "m3g/Background.h"
#include "m3g/Camera.h"
#include "m3g/Graphics3D.h"
#include "m3g/Transform.h"
#include "m3g/World.h"
#include "runtime/midp/Display.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "runtime/midp/String.h"
#include "runtime/midp/Thread.h"

#ifdef MEDECOMP_PORT
extern "C" void medecomp_want_player_levels(void);
extern "C" bool medecomp_take_player_levels(void);
extern "C" void medecomp_quit_game(void);
extern "C" void medecomp_back_to_launcher(void);
extern "C" void medecomp_solo_bags_menu(int on);
extern "C" int medecomp_solo_bags_choosing(void);
extern "C" int medecomp_challenge_level_begin(const char* stat);
extern "C" int medecomp_challenge_level_poll(int* base, int* text);

namespace {
enum {
    TEXT_CHALLENGE_FAILED = 0x9d8,
    LAUNCH_PLAYER_LEVEL = 6,
    LAUNCH_FAILED_SHOWN = 7,
};
UI::PleaseWaitWindow* s_challengeWait = NULL;
int s_challengeFailText = TEXT_CHALLENGE_FAILED;
bool s_challengeRetry = false;

void challengeFailed(int text, bool retry) {
    s_challengeFailText = text;
    s_challengeRetry = retry;
}
}
#endif

namespace {

const int MENU_CACHE = 2;
const int SOUND_SELECT = 35;
const int SOUND_BACK = 36;
const float SOUND_VOLUME = 1.0f;
const int CITY_ANIM = 0x8e;
const int MAYHEM_FAILED = 2;
const int PENDING_DISMISS_ALL = 3;

const int GROUP_TITLE = 0x26;
const int GROUP_INTRO = 0x2a;
const int GROUP_TICKER = 0x30;
const int GROUP_MAIN = 0x31;
const int MESH_TICKER = 0x58;

bool isMainMenu(SceneMenu::MenuState state) {
    return state == SceneMenu::STATE_MAIN_MENU || state == SceneMenu::STATE_MAIN_MENU_ALT;
}

}

void SceneMenu::stateActivate() {
    m_phase = PHASE_ACTIVE;
    m_stateTime = 0;
}

void SceneMenu::initStateLaunchingChallenge() {
    m_launchStep = 0;
#ifdef MEDECOMP_PORT
    s_challengeWait = static_cast<UI::PleaseWaitWindow*>(
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_PLEASE_WAIT, 0, 0, 0, 0));
    challengeFailed(TEXT_CHALLENGE_FAILED, true);
#endif
}

void SceneMenu::setupNextPendingChallengeNotification() {
    PendingChallenge next = m_engine->getNextPendingChallenge();
    m_pendingWindow = new UI::PendingChallengeWindow(next.m_id, next.m_name);
    m_engine->getWindowStore().pushWindow(m_pendingWindow);
}

void SceneMenu::initStatePendingChallenges() {
    setupNextPendingChallengeNotification();
}

void SceneMenu::initAchievements() {
    m_achievements->clearClosed();
    m_engine->getWindowStore().pushWindow(m_achievements);
}

void SceneMenu::initGameCompleteResults() {
    AppEngine::getAchievementData()->registerGameEnd();
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_END_OF_GAME_RESULTS, 0, 0, 0, 0);
}

void SceneMenu::initLanguage() {
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_LANGUAGE, 0, 0, 0, 0);
}

void SceneMenu::initUnlockables() {
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_UNLOCKABLES, 0, 0, 0, 0);
}

void SceneMenu::initAboutMenu() {
    if (m_aboutMenu == NULL) {
        m_aboutMenu = static_cast<UI::AboutMenu*>(
            m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_ABOUT_MENU, 0, 0, 0, 0));
        return;
    }
    m_aboutMenu->setHidden(false);
}

void SceneMenu::initHelp() {
#ifdef MEDECOMP_PORT
    if (medecomp_take_player_levels()) return;
#endif
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_HELP, 0, 0, 0, 0);
}

void SceneMenu::initGameOptions() {
    m_gameOptions = m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_GAME_OPTIONS, 0, 0, 0, 0);
}

void SceneMenu::initSoundOptions() {
    m_soundOptions = static_cast<UI::SoundOptions*>(
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_SOUND_OPTIONS, 0, 0, 0, 0));
}

void SceneMenu::initLevelSelectMultiplayer() {
#ifdef MEDECOMP_PORT
    bool choose = port::net::takeChooseNext();
    if (choose) {
        medecomp_solo_bags_menu(0);
    }
    if (choose || medecomp_solo_bags_choosing()) {
        m_levelWindow = m_engine->getWindowStore().createWindow(
            UI::WindowStore::WINDOW_CHAPTER_SELECT_MULTIPLAYER, 0, 0, 0, 0);
        return;
    }
    m_levelWindow = m_engine->getWindowStore().pushWindow(port::net::createLobbyWindow());
#else
    m_levelWindow = m_engine->getWindowStore().createWindow(
        UI::WindowStore::WINDOW_CHAPTER_SELECT_MULTIPLAYER, 0, 0, 0, 0);
#endif
}

void SceneMenu::initLevelSelectSpeedRun() {
    m_levelWindow =
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_CHAPTER_SELECT_SPEEDRUN, 0, 0, 0, 0);
}

void SceneMenu::initLevelSelectStory() {
    m_levelWindow =
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_CHAPTER_SELECT_STORY, 0, 0, 0, 0);
}

void SceneMenu::deinitLanguage() {
    m_engine->initAboutString();
}

void SceneMenu::initAbout() {
    SpywareManager::getInstance()->trackViewAbout();
    m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_ABOUT, 0, 0, 0, 0);
}

void SceneMenu::initMediaPicker() {
    midp::Display* display = midp::Display::getDisplay(m_engine->getMIDlet());
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->suspend();
    }
    m_engine->getMediaPicker()->activate(display);
}

void SceneMenu::deinitGameIntro() {
    AppEngine::getCanvas()->getQuadManager().setGroupVisible(GROUP_INTRO, false);
}

void SceneMenu::deinitMainMenu() {
    m_engine->getQuadManager().setGroupVisible(GROUP_MAIN, false);
    m_engine->getQuadManager().setGroupVisible(GROUP_TICKER, false);
}

void SceneMenu::deinitTitle() {
    m_engine->getQuadManager().setGroupVisible(GROUP_TITLE, false);
}

void SceneMenu::initTitle() {
    m_engine->getQuadManager().setGroupVisible(GROUP_TITLE, true);
    m_engine->startFadeIn(false);
}

void SceneMenu::pointerReleaseGameIntro(int, int, int) {
    QuadManager& quads = m_engine->getQuadManager();
    if (!quads.isAnimating(0) && quads.getAnimCurrentFrameIndex(0) == 1) {
        quads.playAnimAcross(0, 2, 1, 5);
    }
}

void SceneMenu::renderGameIntro(midp::Graphics* g) {
    QuadManager& quads = m_engine->getQuadManager();
    TextManager* text = m_engine->getTextManager();
    quads.render(g);
    m_introText.draw(g, 40, 80, 0x41);
    text->drawString(g, 0x916, 10, 280, 360, 0x41);
    text->drawString(g, 0x917, 10, 310, 400, 0x41);
    quads.render(g, 0x80);
    if (quads.getAnimCurrentFrameIndex(0) != 5 && !quads.isAnimating(0)) {
        int x = m_engine->getWidth() - 50;
        int y = m_introText.getWrappedTextHeight() + 90;
        text->drawString(g, 0x813, 11, x, y, 12);
    }
}

void SceneMenu::initGameIntro() {
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    quads.setGroupVisible(GROUP_INTRO, true);
    quads.playAnimAcross(0, 2, 0, 1);
    m_introText.wrapString(0x915, 10, m_engine->getWidth() - 40);
    m_engine->startFadeIn(true);
}

void SceneMenu::renderTitle(midp::Graphics* g) {
    m_engine->getQuadManager().render(g);
    if (m_titleReady) {
        m_engine->getTextManager()->drawString(g, 0x813, 9, m_engine->getWidth() >> 1,
                                               m_engine->getHeight() - 0x55, 0x42);
    }
}

void SceneMenu::pointerDraggedMainMenu(int x, int y, int) {
    m_mainMenu.pointerDragged(x, y, m_engine->getWidth(), m_engine->getHeight());
}

void SceneMenu::pointerDragged(int x, int y, int id) {
    if (!isMainMenu(m_state)) {
        return;
    }
    pointerDraggedMainMenu(x, y, id);
}

void SceneMenu::pointerPressedMainMenu(int x, int y, int) {
    m_mainMenu.pointerPressed(x, y, m_engine->getWidth(), m_engine->getHeight());
}

void SceneMenu::pointerPressed(int x, int y, int id, int) {
    if (!isMainMenu(m_state)) {
        return;
    }
    pointerPressedMainMenu(x, y, id);
}

void SceneMenu::renderMainMenu(midp::Graphics* g) {
    m_mainMenu.render(g);
    if (m_tickerIndex == -1) {
        return;
    }
    ea::EASpywareManager* spyware = ea::EASpywareManager::getInstance();
    int count = spyware->getTickerCount();
    midp::ReferenceCountedPointer<midp::String> current(spyware->getTickerString(m_tickerIndex));
    int next = (m_tickerIndex + 1) % count;
    midp::ReferenceCountedPointer<midp::String> following(spyware->getTickerString(next));
    TextManager* text = m_engine->getTextManager();
    text->drawString(g, current, 6, (int)m_tickerX, 2, 9);
    text->drawString(g, following, 6, (int)m_tickerX2, 2, 9);
}

void SceneMenu::initTickers() {
    ea::EASpywareManager* spyware = ea::EASpywareManager::getInstance();
    if (spyware->getTickerCount() == 0) {
        m_tickerIndex = -1;
        return;
    }
    m_tickerIndex = 0;
    midp::ReferenceCountedPointer<midp::String> first(spyware->getTickerString(0));
    int textWidth = m_engine->getTextManager()->getStringWidth(first, 6);
    m_tickerGap = m_engine->getWidth() - 100;
    float width = (float)m_engine->getWidth();
    m_tickerX = width;
    m_tickerX2 = width + (float)textWidth + (float)m_tickerGap;
    m_tickerUrl = spyware->getTickerURL(m_tickerIndex);
    m_engine->getQuadManager().setGroupVisible(GROUP_TICKER, true);
    m_engine->getQuadManager().setMeshVisible(MESH_TICKER, true);
}

void SceneMenu::initMainMenu() {
    LevelData* levels = AppEngine::getLevelData();
    m_mainMenu.create();
    int unlocked = levels->getNumUnlockedLevels();
    MenuStringChoice* play = m_mainMenu.getSubMenu(0);
    int selected = play->getSelectedItem();
    play->create(5, 0x906);
    play->append(0x81a);
    if (unlocked != 0) {
        play->append(0x819);
    }
    if (levels->calcNextContinueLevel() != -1) {
        play->append(0x818);
    }
    if (levels->isStoryLevelComplete()) {
        play->append(0x81c);
    }
#ifdef MEDECOMP_PORT
    play->append(MenuMain::ITEM_SOLO_BAGS);
#else
    play->append(0x930);
#endif
    play->setSelectedItem(selected);
    m_mainMenu.wrapStrings(true);
    m_mainMenu.reset();
    if (m_openSubMenu != -1) {
        m_mainMenu.setSubMenuOpen(m_openSubMenu, m_openItem);
    }
    m_engine->getQuadManager().setGroupVisible(GROUP_MAIN, true);
    initTickers();
}

void SceneMenu::deinitLoading() {
    AnimationManager::unloadImage(0xe7);
}

void SceneMenu::renderLoading(midp::Graphics* g) {
    int width = m_engine->getWidth();
    int height = m_engine->getHeight();
    AnimationManager::setColor(g, 1);
    g->fillRect(0, 0, width, height);
    m_engine->getLoadingRunAnimPlayer()->drawAnim(g, (width >> 3) * 7, (height >> 3) * 7);
}

void SceneMenu::initLoading() {
    AnimationManager::loadImage(m_engine->getResourceManager(), 0xe7);
}

void SceneMenu::deinitState() {
    switch (m_state) {
    case STATE_LOADING:
        deinitLoading();
        break;
    case STATE_TITLE:
        deinitTitle();
        break;
    case STATE_MAIN_MENU:
    case STATE_MAIN_MENU_ALT:
        deinitMainMenu();
        break;
    case STATE_GAME_INTRO:
        deinitGameIntro();
        break;
    case STATE_SOUND_OPTIONS:
    case STATE_GAME_OPTIONS:
        AppEngine::getCanvas()->saveRMSAppSettings();
        break;
    case STATE_LEADERBOARD:
        if (AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->isAnimating()) {
            AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->hardStop();
        }
        break;
    case STATE_LANGUAGE:
        deinitLanguage();
        break;
    default:
        break;
    }
    unsigned next = (unsigned)m_nextState;
    if (next < 22 && ((1u << (next & 0xff)) & 0x25041c) != 0) {
        return;
    }
    ea::EASpywareManager::getInstance()->hideOrigin();
}

void SceneMenu::initState() {
    UI::WindowStore::WindowType type;
    switch (m_state) {
    case STATE_TITLE:
        initTitle();
        return;
    case STATE_MAIN_MENU:
    case STATE_MAIN_MENU_ALT:
        initMainMenu();
        if (m_mainMenu.m_originHidden) {
            return;
        }
        ea::EASpywareManager::getInstance()->showOrigin(1);
        return;
    case STATE_LEVEL_SELECT_STORY:
        initLevelSelectStory();
        ea::EASpywareManager::getInstance()->showOrigin(2);
        return;
    case STATE_LEVEL_SELECT_SPEEDRUN:
        initLevelSelectSpeedRun();
        return;
    case STATE_LEVEL_SELECT_MULTIPLAYER:
        initLevelSelectMultiplayer();
        return;
    case STATE_GAME_INTRO:
        initGameIntro();
        return;
    case STATE_SOUND_OPTIONS:
        initSoundOptions();
        return;
    case STATE_GAME_OPTIONS:
        initGameOptions();
        return;
    case STATE_ACHIEVEMENTS:
        initAchievements();
        ea::EASpywareManager::getInstance()->showOrigin(3);
        return;
    case STATE_MEDIA_PICKER:
        initMediaPicker();
        return;
    case STATE_HELP:
        initHelp();
        return;
    case STATE_ABOUT_MENU:
        initAboutMenu();
        return;
    case STATE_ABOUT:
        initAbout();
        return;
    case STATE_LEADERBOARD_WAIT:
        type = UI::WindowStore::WINDOW_PLEASE_WAIT;
        break;
    case STATE_LEADERBOARD:
        m_engine->getWindowStore().createWindow(UI::WindowStore::WINDOW_LEADERBOARD, 0, 0, 0, 0);
        ea::EASpywareManager::getInstance()->showOrigin(4);
        return;
    case STATE_MORE_GAMES:
        initMoreGames();
        return;
    case STATE_UNLOCKABLES:
        initUnlockables();
        ea::EASpywareManager::getInstance()->showOrigin(5);
        return;
    case STATE_LANGUAGE:
        initLanguage();
        return;
    case STATE_BROWSER_918:
    case STATE_BROWSER_91A:
    case STATE_BROWSER_919:
        type = UI::WindowStore::WINDOW_EXIT_TO_BROWSER_CONFIRM;
        break;
    case STATE_NEW_GAME_CONFIRM:
        type = UI::WindowStore::WINDOW_NEW_GAME_CONFIRM;
        break;
    case STATE_TUTORIAL_CONFIRM:
        type = UI::WindowStore::WINDOW_TUTORIAL_CONFIRM;
        break;
    case STATE_CHALLENGE_CONFIRM:
        type = UI::WindowStore::WINDOW_CHALLENGE_CONFIRM;
        break;
    case STATE_ENABLE_SHARE_CONFIRM:
        type = UI::WindowStore::WINDOW_ENABLE_SHARE_DATA_CONFIRM;
        break;
    case STATE_DISABLE_SHARE_CONFIRM:
        type = UI::WindowStore::WINDOW_DISABLE_SHARE_DATA_CONFIRM;
        break;
    case STATE_SHARE_ENABLED:
        SpywareManager::getInstance()->setEnabled(true);
        type = UI::WindowStore::WINDOW_SHARE_DATA_ENABLED_CONFIRM;
        break;
    case STATE_SHARE_DISABLED:
        SpywareManager::getInstance()->setEnabled(false);
        type = UI::WindowStore::WINDOW_SHARE_DATA_DISABLED_CONFIRM;
        break;
    case STATE_GAME_COMPLETE:
        initGameCompleteResults();
        return;
    case STATE_PENDING_CHALLENGES:
        initStatePendingChallenges();
        return;
    case STATE_LAUNCHING_CHALLENGE:
        initStateLaunchingChallenge();
        return;
    case STATE_NO_STORAGE:
        type = UI::WindowStore::WINDOW_NO_STORAGE;
        break;
    default:
        return;
    }
    m_engine->getWindowStore().createWindow(type, 0, 0, 0, 0);
}

void SceneMenu::initMoreGames() {
    ea::EASpywareManager::getInstance()->showMoreGames();
    stateTransition(STATE_MAIN_MENU);
    stateDeactivate();
}

void SceneMenu::stateDeactivate() {
    deinitState();
    m_prevState = m_state;
    m_state = m_nextState;
    m_phase = PHASE_ACTIVATE;
    m_stateTime = 0;
    if (m_fade) {
        m_engine->startFadeIn(true);
    }
    initState();
}

void SceneMenu::stateTransition(MenuState state, bool fade) {
    if (m_state == state) {
        return;
    }
    m_stateTime = 0;
    m_nextState = state;
    if (m_state == STATE_NONE) {
        stateDeactivate();
        return;
    }
    m_fade = fade;
    if (fade) {
        m_engine->startFadeOut(true);
    }
    m_phase = PHASE_DEACTIVATE;
}

void SceneMenu::startNewGame(bool tutorial) {
    SpywareManager::getInstance()->trackNewGame();
    LevelData* levels = AppEngine::getLevelData();
    levels->resetLevelData();
    levels->setCurrentLevelByIndex(LevelData::GAME_MODE_STORY, tutorial ? 0 : 2);
    stateTransitionFade(STATE_TRANSITION_TO_GAME);
}

void SceneMenu::pointerReleaseTitle(int, int, int) {
    if (m_titleReady && m_nextState != STATE_MAIN_MENU) {
        stateTransitionFade(STATE_MAIN_MENU);
    }
}

void SceneMenu::updateStateLaunchingChallenge(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase != PHASE_ACTIVE) {
        if (m_phase == PHASE_ACTIVATE) {
            stateActivate();
        }
        return;
    }
    switch (m_launchStep) {
    case 0:
        if (m_engine->m_mayhemToken != NULL && m_engine->m_mayhemToken->IsValid()) {
            m_getChallenge = new Mayhem::GetChallenge(m_engine->m_mayhemToken.unsafeGetWrappedPointer(),
                                                      m_engine->m_mayhemId, m_engine->getChallengeID());
            m_launchStep = 4;
            return;
        }
#ifdef MEDECOMP_PORT
        challengeFailed(TEXT_CHALLENGE_FAILED, false);
        m_launchStep = 5;
        return;
#endif
        {
            FBConnection* fb = NULL;
            if (!fb->isLoggedIn()) {
                fb->login();
                m_launchStep = 1;
                return;
            }
            m_userRequest = new Mayhem::GetUserRequest(fb->m_userId, true);
            m_launchStep = 2;
            return;
        }
    case 1: {
        FBConnection* fb = NULL;
        if (fb->GetLoginResult() != FBConnection::LOGIN_SUCCEEDED) {
            m_launchStep = 5;
            return;
        }
        m_userRequest = new Mayhem::GetUserRequest(fb->m_userId, true);
        m_launchStep = 2;
        return;
    }
    case 2:
        if (m_userRequest->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
            m_engine->m_mayhemId = m_userRequest->GetMayhemID();
            FBConnection* fb = NULL;
            m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, fb->m_unk1c);
            m_launchStep = 3;
            return;
        }
        if (m_userRequest->GetResult() == MAYHEM_FAILED) {
            m_launchStep = 5;
        }
        return;
    case 3:
        if (m_engine->m_mayhemToken->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
            m_getChallenge = new Mayhem::GetChallenge(m_engine->m_mayhemToken.unsafeGetWrappedPointer(),
                                                      m_engine->m_mayhemId, m_engine->getChallengeID());
            m_launchStep = 4;
            return;
        }
        if (m_engine->m_mayhemToken->GetResult() == MAYHEM_FAILED) {
            m_launchStep = 5;
        }
        return;
    case 4:
        if (m_getChallenge->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
            void* data = NULL;
            int length = m_getChallenge->GetDecodedData(&data);
            if (length > 0) {
                midp::String* name = new midp::String("ChallengeGhost");
                midp::ReferenceCountedPointer<midp::OutputStream> out(
                    midp::OutputStream::getResourceAsStream(name));
                if (out != NULL) {
                    out->write(data, length);
                    out->close();
                }
            }
            std::string stat(m_getChallenge->GetStatTypeCode());
            int level;
#ifdef MEDECOMP_PORT
            if (medecomp_challenge_level_begin(stat.c_str())) {
                m_engine->setChallengeTime(m_getChallenge->GetValue());
                m_launchStep = LAUNCH_PLAYER_LEVEL;
                return;
            }
            if (sscanf(stat.c_str(), "challenge_level%d", &level) != 1 || level < 1 ||
                level > AppEngine::getLevelData()->getLevelNum()) {
                challengeFailed(TEXT_CHALLENGE_FAILED, false);
                m_launchStep = 5;
                return;
            }
#else
            sscanf(stat.c_str(), "challenge_level%d", &level);
#endif
            level -= 1;
            m_engine->setChallengeTime(m_getChallenge->GetValue());
            AppEngine::getLevelData()->setCurrentLevelByIndex(LevelData::GAME_MODE_CHALLENGE, level);
#ifdef MEDECOMP_PORT
            if (s_challengeWait != NULL) {
                s_challengeWait->close(UI::RESULT_NONE);
                s_challengeWait = NULL;
            }
#endif
            m_engine->getSceneMenu()->stateTransitionFade(STATE_TRANSITION_TO_GAME);
            return;
        }
        if (m_getChallenge->GetResult() == MAYHEM_FAILED) {
            m_launchStep = 5;
        }
        return;
    case 5:
#ifdef MEDECOMP_PORT
        if (s_challengeWait != NULL) {
            if (s_challengeRetry) {
                s_challengeWait->Failed(s_challengeFailText);
            } else {
                s_challengeWait->Succeeded(s_challengeFailText);
            }
            m_launchStep = LAUNCH_FAILED_SHOWN;
            return;
        }
#endif
        stateTransition(STATE_CHALLENGE_FAILED);
        stateDeactivate();
        return;
#ifdef MEDECOMP_PORT
    case LAUNCH_PLAYER_LEVEL: {
        int base = -1, text = TEXT_CHALLENGE_FAILED;
        int ready = medecomp_challenge_level_poll(&base, &text);
        if (ready == 0) {
            return;
        }
        if (ready < 0) {
            challengeFailed(text, ready == -2);
            m_launchStep = 5;
            return;
        }
        AppEngine::getLevelData()->setCurrentLevelByIndex(LevelData::GAME_MODE_CHALLENGE, base);
        if (s_challengeWait != NULL) {
            s_challengeWait->close(UI::RESULT_NONE);
            s_challengeWait = NULL;
        }
        m_engine->getSceneMenu()->stateTransitionFade(STATE_TRANSITION_TO_GAME);
        return;
    }
    case LAUNCH_FAILED_SHOWN: {
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        s_challengeWait = NULL;
        if (result == UI::PleaseWaitWindow::BUTTON_RETRY) {
            initStateLaunchingChallenge();
            return;
        }
        stateTransition(STATE_MAIN_MENU);
        return;
    }
#endif
    default:
        return;
    }
}

void SceneMenu::updateStatePendingChallenges(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase != PHASE_ACTIVE) {
        if (m_phase == PHASE_ACTIVATE) {
            stateActivate();
        }
        return;
    }
    int result = m_engine->getWindowStore().getWindowResult();
    if (result == UI::RESULT_NONE) {
        return;
    }
    m_engine->getWindowStore().clearWindowResult();
    MenuState next;
    if (result == PENDING_DISMISS_ALL) {
        m_engine->clearPendingChallenges();
        next = STATE_MAIN_MENU;
    } else if (result == UI::RESULT_CANCEL) {
        m_engine->popPendingChallenge();
        if (m_engine->getPendingChallengeCount() > 0) {
            setupNextPendingChallengeNotification();
            return;
        }
        next = STATE_MAIN_MENU;
    } else if (result == UI::RESULT_OK) {
        m_engine->popPendingChallenge();
        m_engine->setChallengeID(m_pendingWindow->getChallengeID());
        next = STATE_LAUNCHING_CHALLENGE;
    } else {
        return;
    }
    stateTransition(next);
}

void SceneMenu::updateGameCompleteResults(int) {
    if (m_phase == PHASE_DEACTIVATE && m_engine->isFadedOut()) {
        stateDeactivate();
        return;
    }
    if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
        return;
    }
    m_engine->getWindowStore().clearWindowResult();
    stateTransitionFade(STATE_MAIN_MENU);
}

void SceneMenu::updateTransitionToGame(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase != PHASE_ACTIVE) {
        if (m_phase == PHASE_ACTIVATE) {
            stateActivate();
        }
        return;
    }
    m_engine->changeScene(3);
}

void SceneMenu::updateMediaPicker(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        midp::Display* display = midp::Display::getDisplay(m_engine->getMIDlet());
        if (display->getCurrent() != m_engine) {
            return;
        }
        BGMusic* music = m_engine->getBGMusic();
        if (music != NULL) {
            music->resume();
        }
        stateTransition(STATE_MAIN_MENU);
        m_engine->startFadeIn(true);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateGameIntro(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (!m_engine->isFadedOut()) {
            return;
        }
        SpywareManager::getInstance()->trackNewGame();
        LevelData* levels = AppEngine::getLevelData();
        levels->resetLevelData();
        levels->setCurrentLevelByIndex(LevelData::GAME_MODE_STORY, 0);
        stateTransition(STATE_TRANSITION_TO_GAME);
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        QuadManager& quads = m_engine->getQuadManager();
        if (quads.getAnimCurrentFrameIndex(0) == 5 && !quads.isAnimating(0)) {
            m_engine->startFadeOut(false, 0xffffff);
            m_phase = PHASE_DEACTIVATE;
            return;
        }
        quads.updateAnim(0, dt);
        return;
    }
    if (m_phase == PHASE_ACTIVATE && m_engine->isFadedIn()) {
        stateActivate();
    }
}

void SceneMenu::updateTitle(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (!m_engine->isFading()) {
            stateDeactivate();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        m_titleReady = true;
        return;
    }
    if (m_phase == PHASE_ACTIVATE && !m_engine->isFading()) {
        stateActivate();
    }
}

void SceneMenu::calculateCameraOrientationAtTime(int time, MathVector& position, float& yawDeg,
                                                 float& pitchDeg) {
    m_cameraPosAnim->setAnimTime(time);
    m_cameraLookAnim->setAnimTime(time);
    float from[3];
    float at[3];
    m_cameraPosNode->getTranslation(from);
    m_cameraLookNode->getTranslation(at);
    position.set(from[0], from[1], from[2]);
    MathTrig::convertLookVectorToEulerRotationsDeg(at[0] - from[0], at[1] - from[1], at[2] - from[2], yawDeg,
                                                   pitchDeg);
}

void SceneMenu::jerkCityscape() {
    if (m_jerking) {
        return;
    }
    int oldTime = m_cameraPosAnim->getAnimTime();
    int duration = m_cameraPosAnim->getAnimDuration();
    int jump = m_engine->rand(duration >> 1);
    int newTime = (oldTime + (duration >> 2) + jump) % duration;
    m_cameraPosAnim->setAnimTime(newTime);
    m_cameraLookAnim->setAnimTime(newTime);
    m_jerking = true;
    m_jerkT = 1.0f;
    calculateCameraOrientationAtTime(oldTime, m_jerkFrom, m_jerkFromYaw, m_jerkFromPitch);
    calculateCameraOrientationAtTime(newTime, m_jerkTo, m_jerkToYaw, m_jerkToPitch);
}

void SceneMenu::processMenuSelect() {
    MenuMainSubMenu* selected = m_mainMenu.getSelectedSubMenu();
    m_openSubMenu = m_mainMenu.getSelectionIndex();
    m_openItem = selected->getSelectedIndex();
    if (isMainMenu(m_state)) {
        int item = m_mainMenu.getSelectedSubMenu()->getSelectedItem();
        if (item == MenuMain::ITEM_MULTIPLAYER) {
#ifdef MEDECOMP_PORT
            medecomp_solo_bags_menu(0);
#endif
            stateTransitionFade(STATE_LEVEL_SELECT_MULTIPLAYER);
#ifdef MEDECOMP_PORT
        } else if (item == MenuMain::ITEM_SOLO_BAGS) {
            medecomp_solo_bags_menu(1);
            stateTransitionFade(STATE_LEVEL_SELECT_MULTIPLAYER);
        } else if (item == MenuMain::ITEM_PLAYER_LEVELS) {
            medecomp_want_player_levels();
            stateTransition(STATE_HELP);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_QUIT) {
            medecomp_quit_game();
        } else if (item == MenuMain::ITEM_LAUNCHER) {
            medecomp_back_to_launcher();
#endif
        } else if (item == MenuMain::ITEM_WALLPAPERS) {
            stateTransition(STATE_UNLOCKABLES);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_LANGUAGE) {
            stateTransition(STATE_LANGUAGE);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_SOUND) {
            SpywareManager::getInstance()->trackOptionsSound();
            stateTransition(STATE_SOUND_OPTIONS);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_ABOUT) {
            stateTransition(STATE_ABOUT_MENU);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_HELP) {
            stateTransition(STATE_HELP);
            jerkCityscape();
        } else if (item == 0x818) {
            SpywareManager::getInstance()->trackContinueGame();
            LevelData* levels = AppEngine::getLevelData();
            levels->setCurrentLevelByIndex(LevelData::GAME_MODE_STORY, levels->calcNextContinueLevel());
            stateTransitionFade(STATE_TRANSITION_TO_GAME);
        } else if (item == 0x819) {
            stateTransitionFade(STATE_LEVEL_SELECT_STORY);
        } else if (item == 0x81a) {
            if (AppEngine::getLevelData()->getNumUnlockedLevels() == 0) {
                stateTransition(STATE_GAME_INTRO);
            } else {
                stateTransition(STATE_NEW_GAME_CONFIRM);
            }
        } else if (item == 0x81c) {
            stateTransitionFade(STATE_LEVEL_SELECT_SPEEDRUN);
        } else if (item == MenuMain::ITEM_LEADERBOARDS) {
            stateTransition(STATE_LEADERBOARD);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_SETTINGS) {
            stateTransition(STATE_GAME_OPTIONS);
            jerkCityscape();
        } else if (item == MenuMain::ITEM_BADGES) {
            SpywareManager::getInstance()->trackViewBadges();
            stateTransition(STATE_ACHIEVEMENTS);
            jerkCityscape();
        } else if (item == 0x826) {
            SpywareManager::getInstance()->trackSelectMediaPicker();
            stateTransition(STATE_MEDIA_PICKER);
        } else if (item == 0x827) {
            m_mainMenu.reset();
            jerkCityscape();
        }
    }
    m_engine->getSoundManager().playEvent(SOUND_SELECT, SOUND_VOLUME);
}

void SceneMenu::updateMainMenu(int dt) {
    if (m_engine->getPendingChallengeCount() >= 1) {
        stateTransition(STATE_PENDING_CHALLENGES);
        stateDeactivate();
        return;
    }
    m_mainMenu.update(dt);
    if (m_phase == PHASE_DEACTIVATE) {
        if (m_engine->isFading()) {
            return;
        }
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (!m_engine->isFading() && m_mainMenu.isItemSelected()) {
            processMenuSelect();
        }
        if (m_tickerIndex == -1) {
            initTickers();
            return;
        }
        ea::EASpywareManager* spyware = ea::EASpywareManager::getInstance();
        int count = spyware->getTickerCount();
        midp::ReferenceCountedPointer<midp::String> current(spyware->getTickerString(m_tickerIndex));
        int nextIndex = (m_tickerIndex + 1) % count;
        midp::ReferenceCountedPointer<midp::String> next(spyware->getTickerString(nextIndex));
        TextManager* text = m_engine->getTextManager();
        int widthCurrent = text->getStringWidth(current, 6);
        int widthNext = text->getStringWidth(next, 6);
        float step = (float)dt * -0.05f;
        m_tickerX += step;
        m_tickerX2 += step;
        if (m_tickerX < (float)(-widthCurrent)) {
            m_tickerX = m_tickerX2;
            m_tickerIndex = (m_tickerIndex + 1) % count;
            m_tickerX2 = m_tickerX + (float)widthNext + (float)m_tickerGap;
            m_tickerUrl = spyware->getTickerURL(m_tickerIndex);
        }
        return;
    }
    if (m_phase == PHASE_ACTIVATE && !m_engine->isFading()) {
        stateActivate();
    }
}

void SceneMenu::pointerReleaseMainMenu(int x, int y, int) {
    int width = m_engine->getWidth();
    int height = m_engine->getHeight();
    if (m_mainMenu.pointerReleased(x, y, width, height)) {
        jerkCityscape();
    }
    if (y <= 14 && m_tickerIndex != -1) {
        ea::EASpywareManager::getInstance()->logEvent(0x753c);
        midp::RuntimeIPhone::launchBrowser(m_tickerUrl);
    }
}

void SceneMenu::pointerReleased(int x, int y, int id) {
    if (m_state > STATE_EXIT) {
        if (m_state == STATE_MAIN_MENU_ALT) {
            pointerReleaseMainMenu(x, y, id);
        }
        return;
    }
    if (m_state == STATE_TITLE) {
        pointerReleaseTitle(x, y, id);
        return;
    }
    if (m_state == STATE_MAIN_MENU) {
        pointerReleaseMainMenu(x, y, id);
        return;
    }
    if (m_state == STATE_GAME_INTRO) {
        pointerReleaseGameIntro(x, y, id);
    }
}

void SceneMenu::renderCityscape(midp::Graphics* g) {
    if (m_world == NULL) {
        return;
    }
    int width = m_engine->getWidth();
    int height = m_engine->getHeight();
    m3g::Graphics3D* g3d = AppEngine::getCanvas()->getGraphics3D();
    g3d->bindTarget(g);
    g3d->setViewport(0, 0, width, height);
    g3d->render(m_world);
    g3d->releaseTarget();
}

void SceneMenu::renderLanguage(midp::Graphics* g) {
    renderCityscape(g);
}

void SceneMenu::renderUnlockables(midp::Graphics* g) {
    renderCityscape(g);
}

void SceneMenu::render(midp::Graphics* g) {
    switch (m_state) {
    case STATE_LOADING:
        renderLoading(g);
        break;
    case STATE_TITLE:
        renderCityscape(g);
        renderTitle(g);
        break;
    case STATE_MAIN_MENU:
    case STATE_MAIN_MENU_ALT:
        renderCityscape(g);
        renderMainMenu(g);
        break;
    case STATE_LEVEL_SELECT_STORY:
    case STATE_LEVEL_SELECT_SPEEDRUN:
    case STATE_LEVEL_SELECT_MULTIPLAYER:
    case STATE_SOUND_OPTIONS:
    case STATE_GAME_OPTIONS:
    case STATE_ACHIEVEMENTS:
    case STATE_HELP:
    case STATE_ABOUT_MENU:
    case STATE_ABOUT:
    case STATE_LEADERBOARD_WAIT:
    case STATE_LEADERBOARD:
    case STATE_BROWSER_918:
    case STATE_BROWSER_91A:
    case STATE_BROWSER_919:
    case STATE_NEW_GAME_CONFIRM:
    case STATE_TUTORIAL_CONFIRM:
    case STATE_CHALLENGE_CONFIRM:
    case STATE_ENABLE_SHARE_CONFIRM:
    case STATE_DISABLE_SHARE_CONFIRM:
    case STATE_SHARE_ENABLED:
    case STATE_SHARE_DISABLED:
    case STATE_GAME_COMPLETE:
    case STATE_PENDING_CHALLENGES:
    case STATE_LAUNCHING_CHALLENGE:
        renderCityscape(g);
        break;
    case STATE_GAME_INTRO:
        renderGameIntro(g);
        break;
    case STATE_UNLOCKABLES:
        renderUnlockables(g);
        break;
    case STATE_LANGUAGE:
        renderLanguage(g);
        break;
    case STATE_NO_STORAGE:
        g->setColor(0xffffff);
        g->fillRect(0, 0, m_engine->getWidth(), m_engine->getHeight());
        break;
    default:
        break;
    }
}

void SceneMenu::updateCityscape(int dt) {
    if (!m_jerking) {
        m_cameraPosAnim->updateAnim(dt >> 2);
        m_cameraLookAnim->updateAnim(dt >> 2);
    } else {
        m_jerkT += (float)dt * -0.002f;
        if (!(m_jerkT > 0.0f)) {
            m_jerking = false;
        }
    }
    if (m_jerking) {
        float t = 1.0f - m_jerkT;
        float yaw = m_jerkT * m_jerkFromYaw + m_jerkToYaw * t;
        float pitch = m_jerkT * m_jerkFromPitch + m_jerkToPitch * t;
        MathVector pos;
        pos.setAsLinearInterpolation(m_jerkFrom, m_jerkTo, t);
        m_cameraTransform->setIdentity();
        m_cameraTransform->postTranslate(pos.x, pos.y, pos.z);
        m_cameraTransform->postRotate(yaw, 0.0f, 1.0f, 0.0f);
        m_cameraTransform->postRotate(pitch, 1.0f, 0.0f, 0.0f);
    } else {
        float from[3];
        float at[3];
        m_cameraPosNode->getTranslation(from);
        m_cameraLookNode->getTranslation(at);
        GameCamera::createLookAtTransform(m_cameraTransform, from[0], from[1], from[2], at[0], at[1], at[2], 0.0f,
                                          1.0f, 0.0f);
    }
    m_cameraTransform->postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    m_camera->setTransform(m_cameraTransform);
}

void SceneMenu::updateChallengeConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_LEADERBOARD);
            return;
        }
        if (result != UI::RESULT_OK) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransitionFade(STATE_TRANSITION_TO_GAME);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateTutorialConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (m_engine->isFadedOut()) {
            stateDeactivate();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->isFadedOut()) {
            stateTransition(STATE_GAME_INTRO);
        }
        updateCityscape(dt);
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            startNewGame(false);
            return;
        }
        if (result != UI::RESULT_OK) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        startNewGame(true);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateShareDataDisabled(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_ABOUT_MENU);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateShareDataEnabled(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_ABOUT_MENU);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateDisableShareDataConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_ABOUT_MENU);
            return;
        }
        if (result != UI::RESULT_OK) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_SHARE_DISABLED);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateEnableShareDataConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_ABOUT_MENU);
            return;
        }
        if (result != UI::RESULT_OK) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_SHARE_ENABLED);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateNewGameConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        int result = m_engine->getWindowStore().getWindowResult();
        if (result == UI::RESULT_CANCEL) {
            m_engine->getWindowStore().clearWindowResult();
            stateTransition(STATE_MAIN_MENU);
            return;
        }
        if (result != UI::RESULT_OK) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_TUTORIAL_CONFIRM);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateExitConf(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase != PHASE_ACTIVE) {
        if (m_phase == PHASE_ACTIVATE) {
            stateActivate();
        }
        return;
    }
    updateCityscape(dt);
    int result = m_engine->getWindowStore().getWindowResult();
    if (result == UI::RESULT_CANCEL) {
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_ABOUT_MENU);
        return;
    }
    if (result != UI::RESULT_OK) {
        return;
    }
    m_engine->getWindowStore().clearWindowResult();
    midp::ReferenceCountedPointer<midp::String> url;
    int textId = 0;
    if (m_state == STATE_BROWSER_919) {
        textId = 0x919;
    } else if (m_state == STATE_BROWSER_91A) {
        textId = 0x91a;
    } else if (m_state == STATE_BROWSER_918) {
        textId = 0x918;
    }
    if (textId != 0) {
        url = m_engine->getTextManager()->getString(textId);
    }
    midp::RuntimeIPhone::launchBrowser(url);
    stateTransition(STATE_ABOUT_MENU);
}

void SceneMenu::end() {
    if (m_loadingThread != NULL) {
        midp::ReferenceCountedPointer<midp::Thread> thread(m_loadingThread);
        m_loadingThreadState = LOADING_EXIT;
        m_loadingThread = NULL;
        thread->join();
    }
    m_engine->getQuadManager().freeQuads(MENU_CACHE);
}

void SceneMenu::resume() {
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->resume();
    }
    if (m_state == STATE_LOADING && m_loadingThreadState == LOADING_PAUSED) {
        m_loadingThreadState = LOADING_RUN;
    }
    ea::EASpywareManager::getInstance()->MTXresume();
}

void SceneMenu::pause() {
    MenuMainSubMenu* selected = m_mainMenu.getSelectedSubMenu();
    if (selected != NULL) {
        selected->transitionToIdle();
    }
    BGMusic* music = m_engine->getBGMusic();
    if (music != NULL) {
        music->suspend();
    }
    if (m_state == STATE_LOADING && m_loadingThreadState != LOADING_EXIT) {
        m_loadingThreadState = LOADING_PAUSED;
    }
    ea::EASpywareManager::getInstance()->MTXpause();
}

void SceneMenu::precacheAchievementsWindow() {
    if (m_achievements != NULL) {
        delete m_achievements;
        m_achievements = NULL;
    }
    m_achievements = new UI::AchievementWindow();
    m_achievements->setDeleteOnClosed(false);
}

void SceneMenu::processMenuBack() {
    switch (m_state) {
    case STATE_LEVEL_SELECT_STORY:
    case STATE_LEVEL_SELECT_SPEEDRUN:
    case STATE_LEVEL_SELECT_MULTIPLAYER:
        stateTransitionFade(STATE_MAIN_MENU);
        break;
    case STATE_ABOUT:
        stateTransition(STATE_ABOUT_MENU);
        jerkCityscape();
        break;
    case STATE_LANGUAGE:
        m_engine->freeLoadingScreens();
        m_engine->loadLoadingScreens();
        precacheAchievementsWindow();
    case STATE_SOUND_OPTIONS:
    case STATE_GAME_OPTIONS:
    case STATE_ACHIEVEMENTS:
    case STATE_HELP:
    case STATE_ABOUT_MENU:
    case STATE_LEADERBOARD:
    case STATE_UNLOCKABLES:
        stateTransition(STATE_MAIN_MENU);
        jerkCityscape();
        break;
    default:
        break;
    }
    m_engine->getSoundManager().playEvent(SOUND_BACK, SOUND_VOLUME);
}

void SceneMenu::updateLanguage(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        m_engine->saveGameOptions();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateUnlockables(int dt) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        updateCityscape(dt);
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateLeaderboard(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateAbout(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateAboutMenu(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        m_aboutMenu = NULL;
        m_engine->saveGameOptions();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateHelp(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateAchievements(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateGameOptions(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        m_engine->saveGameOptions();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateSoundOptions(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            SoundManager& sound = m_engine->getSoundManager();
            float music = m_soundOptions->getMusicLevel();
            float effects = m_soundOptions->getEffectsLevel();
            float volumeMusic = sound.getVolumeMusic();
            float volumeSfx = sound.getVolumeSFX();
            if (effects != volumeSfx) {
                sound.setVolumeSFX(effects);
                sound.playEvent(SOUND_SELECT, SOUND_VOLUME);
            }
            if (music != volumeMusic) {
                sound.setVolumeMusic(music);
            }
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        processMenuBack();
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateLevelSelectMultiplayer(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (!m_engine->isFading()) {
            stateDeactivate();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        int result = m_engine->getWindowStore().getWindowResult();
        if (result != UI::RESULT_NONE) {
            m_engine->getWindowStore().clearWindowResult();
#ifdef MEDECOMP_PORT
            if (result == port::net::kLobbyChooseLevel) {
                m_levelWindow = m_engine->getWindowStore().createWindow(
                    UI::WindowStore::WINDOW_CHAPTER_SELECT_MULTIPLAYER, 0, 0, 0, 0);
                return;
            }
            if (result == UI::RESULT_OK && !medecomp_solo_bags_choosing()) {
                m_levelWindow = m_engine->getWindowStore().pushWindow(port::net::createLobbyWindow());
                return;
            }
#endif
            processMenuBack();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateLevelSelectSpeedRun(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (!m_engine->isFading()) {
            stateDeactivate();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() != UI::RESULT_NONE) {
            m_engine->getWindowStore().clearWindowResult();
            processMenuBack();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateLevelSelectStory(int) {
    if (m_phase == PHASE_DEACTIVATE) {
        if (!m_engine->isFading()) {
            stateDeactivate();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_engine->getWindowStore().getWindowResult() != UI::RESULT_NONE) {
            m_engine->getWindowStore().clearWindowResult();
            processMenuBack();
        }
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::updateLoadingState(int) {
    int progress;
    switch (m_loadingStep) {
    case 1:
        progress = 0;
        break;
    case 2:
        m_engine->getQuadManager().loadQuads(MENU_CACHE);
        progress = 20;
        break;
    case 3: {
        M3GAssets* assets = AppEngine::getM3GAssets();
        AnimationManager3D& anims = AppEngine::getCanvas()->getAnimationManager3D();
        m3g::World* world = new m3g::World();
        m_world = world;
        float aspect = (float)m_engine->getHeight() / (float)m_engine->getWidth();
        m3g::Camera* camera = new m3g::Camera();
        m_camera = camera;
#ifdef MEDECOMP_PORT
        camera->setPerspective(medecomp_wide_fov(90.0f), medecomp_wide_aspect(m_engine->getHeight()), 5.0f, 550.0f);
#else
        camera->setPerspective(90.0f, aspect, 5.0f, 550.0f);
#endif
        world->addChild(camera);
        world->setActiveCamera(camera);
        m3g::Background* background = new m3g::Background();
        background->setColorClearEnable(true);
        background->setColor(-1);
        background->setDepthClearEnable(true);
        world->setBackground(background);
        M3GAssets::addNode(world, assets->loadModel(0, MENU_CACHE));
        m_cameraTransform = new m3g::Transform();
        m_cameraPosNode = assets->loadNodeCached(1, MENU_CACHE);
        m_cameraPosAnim = new AnimPlayer3D(anims);
        m_cameraPosAnim->setNode(m_cameraPosNode);
        m_cameraPosAnim->startAnim(CITY_ANIM, AnimPlayer3D::START_LOOP | AnimPlayer3D::START_RESTART);
        m_cameraLookNode = assets->loadNodeCached(2, MENU_CACHE);
        m_cameraLookAnim = new AnimPlayer3D(anims);
        m_cameraLookAnim->setNode(m_cameraLookNode);
        m_cameraLookAnim->startAnim(CITY_ANIM, AnimPlayer3D::START_LOOP | AnimPlayer3D::START_RESTART);
        progress = 50;
        break;
    }
    case 4:
        precacheAchievementsWindow();
        m_loadingThreadState = LOADING_EXIT;
        m_loadingThread = NULL;
        if (m_engine->getSaveFileError() == 0) {
            stateTransitionFade(m_startState);
        } else {
            stateTransition(STATE_SAVE_ERROR);
        }
        m_loadingProgress = 100;
        return;
    default:
        return;
    }
    m_loadingProgress = progress;
    m_loadingStep += 1;
}

void SceneMenu::updateLoading(int dt) {
    m_engine->updateLoading(dt);
    m_engine->getLoadingRunAnimPlayer()->updateAnim(dt);
    if (m_phase == PHASE_DEACTIVATE) {
        stateDeactivate();
        return;
    }
    if (m_phase == PHASE_ACTIVE) {
        if (m_loadingThread == NULL && m_loadingThreadState == LOADING_RUN) {
            if (m_engine->isFading()) {
                return;
            }
            m_loadingThread = new midp::Thread(this);
            m_loadingThreadState = LOADING_RUN;
            m_loadingThread->start();
            return;
        }
        midp::Thread::sleep(0x28);
        return;
    }
    if (m_phase == PHASE_ACTIVATE) {
        stateActivate();
    }
}

void SceneMenu::update(int dt) {
    m_stateTime += dt;
    if (!m_mayhemDone) {
        if (m_mayhemStep == 2) {
            if (m_engine->m_mayhemToken->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
                m_mayhemDone = true;
                m_mayhemFailed = false;
                m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, "");
            } else if (m_engine->m_mayhemToken->GetResult() == MAYHEM_FAILED) {
                m_mayhemFailed = true;
            }
        } else if (m_mayhemStep == 1) {
            if (m_userRequest->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
                m_engine->m_mayhemId = m_userRequest->GetMayhemID();
                m_engine->m_mayhemToken = new Mayhem::Token(m_engine->m_mayhemId, "");
                m_mayhemStep = 2;
            } else if (m_userRequest->GetResult() == MAYHEM_FAILED) {
                m_mayhemDone = true;
                m_mayhemFailed = true;
            }
        } else if (m_mayhemStep == 0) {
            if (!m_engine->IsMayhemSessionValid()) {
                m_userRequest = new Mayhem::GetUserRequest(true);
                m_mayhemStep = 1;
            } else {
                m_mayhemDone = true;
                m_mayhemFailed = false;
            }
        }
    }
    switch (m_state) {
    case STATE_LOADING:
        updateLoading(dt);
        break;
    case STATE_TITLE:
        updateCityscape(dt);
        updateTitle(dt);
        break;
    case STATE_MAIN_MENU:
    case STATE_MAIN_MENU_ALT:
        updateCityscape(dt);
        updateMainMenu(dt);
        break;
    case STATE_LEVEL_SELECT_STORY:
        updateCityscape(dt);
        updateLevelSelectStory(dt);
        break;
    case STATE_LEVEL_SELECT_SPEEDRUN:
        updateCityscape(dt);
        updateLevelSelectSpeedRun(dt);
        break;
    case STATE_LEVEL_SELECT_MULTIPLAYER:
        updateCityscape(dt);
        updateLevelSelectMultiplayer(dt);
        break;
    case STATE_GAME_INTRO:
        updateGameIntro(dt);
        break;
    case STATE_SOUND_OPTIONS:
        updateCityscape(dt);
        updateSoundOptions(dt);
        break;
    case STATE_GAME_OPTIONS:
        updateCityscape(dt);
        updateGameOptions(dt);
        break;
    case STATE_ACHIEVEMENTS:
        updateCityscape(dt);
        updateAchievements(dt);
        break;
    case STATE_MEDIA_PICKER:
        updateMediaPicker(dt);
        updateCityscape(dt);
        break;
    case STATE_HELP:
        updateCityscape(dt);
        updateHelp(dt);
        break;
    case STATE_ABOUT_MENU:
        updateCityscape(dt);
        updateAboutMenu(dt);
        break;
    case STATE_ABOUT:
        updateCityscape(dt);
        updateAbout(dt);
        break;
    case STATE_LEADERBOARD_WAIT:
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_LEADERBOARD, false);
        stateDeactivate();
        break;
    case STATE_LEADERBOARD:
        updateCityscape(dt);
        updateLeaderboard(dt);
        break;
    case STATE_UNLOCKABLES:
        updateUnlockables(dt);
        break;
    case STATE_LANGUAGE:
        updateLanguage(dt);
        break;
    case STATE_EXIT:
        if (!m_engine->isFading()) {
            m_engine->endGame();
        }
        break;
    case STATE_TRANSITION_TO_GAME:
        updateTransitionToGame(dt);
        break;
    case STATE_BROWSER_918:
    case STATE_BROWSER_91A:
    case STATE_BROWSER_919:
        updateExitConf(dt);
        break;
    case STATE_NEW_GAME_CONFIRM:
        updateNewGameConf(dt);
        break;
    case STATE_TUTORIAL_CONFIRM:
        updateTutorialConf(dt);
        break;
    case STATE_CHALLENGE_CONFIRM:
        updateChallengeConf(dt);
        break;
    case STATE_ENABLE_SHARE_CONFIRM:
        updateEnableShareDataConf(dt);
        break;
    case STATE_DISABLE_SHARE_CONFIRM:
        updateDisableShareDataConf(dt);
        break;
    case STATE_SHARE_ENABLED:
        updateShareDataEnabled(dt);
        break;
    case STATE_SHARE_DISABLED:
        updateShareDataDisabled(dt);
        break;
    case STATE_GAME_COMPLETE:
        updateCityscape(dt);
        updateGameCompleteResults(dt);
        break;
    case STATE_PENDING_CHALLENGES:
        updateCityscape(dt);
        updateStatePendingChallenges(dt);
        break;
    case STATE_LAUNCHING_CHALLENGE:
        updateCityscape(dt);
        updateStateLaunchingChallenge(dt);
        break;
    case STATE_NO_STORAGE:
        if (m_engine->getWindowStore().getWindowResult() == UI::RESULT_NONE) {
            return;
        }
        m_engine->getWindowStore().clearWindowResult();
        stateTransition(STATE_TITLE);
        stateDeactivate();
        break;
    default:
        break;
    }
}

void SceneMenu::run() {
    while (true) {
        while (m_loadingThreadState == LOADING_RUN) {
            updateLoadingState(0);
        }
        if (m_loadingThreadState == LOADING_EXIT) {
            break;
        }
        midp::Thread::sleep(1000);
    }
}

void SceneMenu::start(int param) {
    LevelData* levels = AppEngine::getLevelData();
    m_engine->saveRMSAppSettings();
    m_loadingStep = 1;
    m_loadingProgress = 0;
    if (levels->isGameNewlyCompleted()) {
        param = STATE_GAME_COMPLETE;
        levels->setGameNotNewlyCompleted();
    }
    m_startState = (MenuState)param;
    m_state = STATE_LOADING;
    SoundManager& sound = m_engine->getSoundManager();
    sound.setVolumeGlobal(SoundManager::MAX_VOLUME);
    sound.setListenerPosition(0.0f, 0.0f, 0.0f);
    sound.setListenerVelocity(0.0f, 0.0f, 0.0f);
    sound.setVolumeGroup(1, SoundManager::MAX_VOLUME);
    m_engine->getBGMusic()->playMusic(0, BGMusic::PLAY_LOOP);
    initLoading();
}

SceneMenu::~SceneMenu() {
    AppEngine::getM3GAssets()->freeCaches(MENU_CACHE);
    delete m_achievements;
}

SceneMenu::SceneMenu(AppEngine* engine)
    : Scene(engine),
      m_phase(PHASE_ACTIVATE),
      m_state(STATE_NONE),
      m_nextState(STATE_NONE),
      m_prevState(STATE_NONE),
      m_stateTime(0),
      m_fade(false),
      m_loadingStep(0),
      m_startState(STATE_LOADING),
      m_loadingThread(NULL),
      m_loadingThreadState(LOADING_RUN),
      m_world(NULL),
      m_camera(NULL),
      m_cameraTransform(NULL),
      m_cameraPosNode(NULL),
      m_cameraLookNode(NULL),
      m_cameraPosAnim(NULL),
      m_cameraLookAnim(NULL),
      m_jerking(false),
      m_jerkT(0.0f),
      m_jerkFromYaw(0.0f),
      m_jerkFromPitch(0.0f),
      m_jerkToYaw(0.0f),
      m_jerkToPitch(0.0f),
      m_titleReady(false),
      m_launchStep(0),
      m_getChallenge(NULL),
      m_levelWindow(NULL),
      m_soundOptions(NULL),
      m_gameOptions(NULL),
      m_achievements(NULL),
      m_aboutMenu(NULL),
      m_openSubMenu(-1),
      m_openItem(-1),
      m_tickerIndex(0),
      m_tickerGap(0),
      m_tickerX(0.0f),
      m_tickerX2(0.0f),
      m_tickerUrl(NULL),
      m_mayhemStep(0),
      m_mayhemDone(false),
      m_mayhemFailed(false),
      m_userRequest(NULL),
      m_pendingWindow(NULL) {}

void SceneMenu::stateTransitionFade(MenuState state) {
    stateTransition(state, true);
}

void SceneMenu::stateTransition(MenuState state) {
    stateTransition(state, false);
}
