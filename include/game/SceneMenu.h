#pragma once

#include "game/MathVector.h"
#include "game/MenuMain.h"
#include "game/MenuStringChoice.h"
#include "game/Scene.h"
#include "game/WrappedString.h"
#include "m3g/OpaqueObject.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"
#include "runtime/util/VectorSE.h"

class AnimPlayer3D;
namespace Mayhem {
class GetChallenge;
class UserRequest;
}
namespace UI {
class AboutMenu;
class AchievementWindow;
class PendingChallengeWindow;
class SoundOptions;
class Window;
}
namespace m3g {
class Camera;
class Node;
class Transform;
class World;
}
namespace midp {
class Graphics;
class String;
class Thread;
}

class SceneMenu : public Scene {
public:
    enum MenuState {
        STATE_NONE = -1,
        STATE_LOADING = 0,
        STATE_SAVE_ERROR = 1,
        STATE_TITLE = 2,
        STATE_MAIN_MENU = 3,
        STATE_LEVEL_SELECT_STORY = 4,
        STATE_LEVEL_SELECT_SPEEDRUN = 5,
        STATE_LEVEL_SELECT_MULTIPLAYER = 6,
        STATE_GAME_INTRO = 7,
        STATE_SOUND_OPTIONS = 8,
        STATE_GAME_OPTIONS = 9,
        STATE_ACHIEVEMENTS = 10,
        STATE_MEDIA_PICKER = 11,
        STATE_HELP = 12,
        STATE_ABOUT_MENU = 13,
        STATE_ABOUT = 14,
        STATE_LEADERBOARD_WAIT = 15,
        STATE_LEADERBOARD = 16,
        STATE_MORE_GAMES = 17,
        STATE_UNLOCKABLES = 18,
        STATE_LANGUAGE = 19,
        STATE_EXIT = 20,
        STATE_MAIN_MENU_ALT = 21,
        STATE_TRANSITION_TO_GAME = 22,
        STATE_BROWSER_918 = 23,
        STATE_BROWSER_91A = 24,
        STATE_BROWSER_919 = 25,
        STATE_NEW_GAME_CONFIRM = 26,
        STATE_TUTORIAL_CONFIRM = 27,
        STATE_CHALLENGE_CONFIRM = 28,
        STATE_ENABLE_SHARE_CONFIRM = 29,
        STATE_DISABLE_SHARE_CONFIRM = 30,
        STATE_SHARE_ENABLED = 31,
        STATE_SHARE_DISABLED = 32,
        STATE_GAME_COMPLETE = 33,
        STATE_PENDING_CHALLENGES = 34,
        STATE_LAUNCHING_CHALLENGE = 35,
        STATE_CHALLENGE_FAILED = 36,
        STATE_NO_STORAGE = 37,
    };

    enum Phase {
        PHASE_ACTIVATE = 1,
        PHASE_ACTIVE = 2,
        PHASE_DEACTIVATE = 3,
    };

    enum LoadingThread {
        LOADING_RUN = 0,
        LOADING_PAUSED = 1,
        LOADING_EXIT = 2,
    };

    explicit SceneMenu(AppEngine* engine);
    virtual ~SceneMenu();

    virtual void run();
    virtual void start(int param);
    virtual void pause();
    virtual void resume();
    virtual void end();
    virtual void render(midp::Graphics* g);
    virtual void pointerDragged(int x, int y, int id);
    virtual void pointerPressed(int x, int y, int id, int);
    virtual void pointerReleased(int x, int y, int id);
    virtual void update(int dt);

    void stateTransition(MenuState state, bool fade);
    void stateTransition(MenuState state);
    void stateTransitionFade(MenuState state);

    void stateActivate();
    void stateDeactivate();
    void initState();
    void deinitState();

    void initLoading();
    void deinitLoading();
    void updateLoading(int dt);
    void updateLoadingState(int dt);
    void renderLoading(midp::Graphics* g);

    void initTitle();
    void deinitTitle();
    void updateTitle(int dt);
    void renderTitle(midp::Graphics* g);
    void pointerReleaseTitle(int x, int y, int id);

    void initMainMenu();
    void deinitMainMenu();
    void updateMainMenu(int dt);
    void renderMainMenu(midp::Graphics* g);
    void pointerPressedMainMenu(int x, int y, int id);
    void pointerDraggedMainMenu(int x, int y, int id);
    void pointerReleaseMainMenu(int x, int y, int id);
    void initTickers();
    void processMenuSelect();
    void processMenuBack();

    void initGameIntro();
    void deinitGameIntro();
    void updateGameIntro(int dt);
    void renderGameIntro(midp::Graphics* g);
    void pointerReleaseGameIntro(int x, int y, int id);

    void initLevelSelectStory();
    void initLevelSelectSpeedRun();
    void initLevelSelectMultiplayer();
    void updateLevelSelectStory(int dt);
    void updateLevelSelectSpeedRun(int dt);
    void updateLevelSelectMultiplayer(int dt);

    void initSoundOptions();
    void initGameOptions();
    void updateSoundOptions(int dt);
    void updateGameOptions(int dt);
    void initAchievements();
    void precacheAchievementsWindow();
    void updateAchievements(int dt);
    void initHelp();
    void updateHelp(int dt);
    void initAboutMenu();
    void updateAboutMenu(int dt);
    void initAbout();
    void updateAbout(int dt);
    void initMediaPicker();
    void updateMediaPicker(int dt);
    void initMoreGames();
    void initUnlockables();
    void updateUnlockables(int dt);
    void initLanguage();
    void deinitLanguage();
    void updateLanguage(int dt);
    void updateLeaderboard(int dt);

    void updateExitConf(int dt);
    void updateNewGameConf(int dt);
    void updateTutorialConf(int dt);
    void updateChallengeConf(int dt);
    void updateEnableShareDataConf(int dt);
    void updateDisableShareDataConf(int dt);
    void updateShareDataEnabled(int dt);
    void updateShareDataDisabled(int dt);
    void startNewGame(bool tutorial);

    void initGameCompleteResults();
    void updateGameCompleteResults(int dt);
    void initStatePendingChallenges();
    void setupNextPendingChallengeNotification();
    void updateStatePendingChallenges(int dt);
    void initStateLaunchingChallenge();
    void updateStateLaunchingChallenge(int dt);
    void updateTransitionToGame(int dt);

    void renderCityscape(midp::Graphics* g);
    void renderUnlockables(midp::Graphics* g);
    void renderLanguage(midp::Graphics* g);
    void updateCityscape(int dt);
    void jerkCityscape();
    void calculateCameraOrientationAtTime(int time, MathVector& position, float& yawDeg, float& pitchDeg);

    m3g::OpaqueObject<util::VectorSE<midp::Object>, 0x18> m_unk0c;
    MenuMain m_mainMenu;
    MenuStringChoice m_stringChoice0;
    MenuStringChoice m_stringChoice1;
    Phase m_phase;
    MenuState m_state;
    MenuState m_nextState;
    MenuState m_prevState;
    int m_stateTime;
    bool m_fade;
    int m_loadingStep;
    MenuState m_startState;
    midp::ReferenceCountedPointer<midp::Thread> m_loadingThread;
    int m_loadingThreadState;
    midp::ReferenceCountedPointer<m3g::World> m_world;
    midp::ReferenceCountedPointer<m3g::Camera> m_camera;
    midp::ReferenceCountedPointer<m3g::Transform> m_cameraTransform;
    midp::ReferenceCountedPointer<m3g::Node> m_cameraPosNode;
    midp::ReferenceCountedPointer<m3g::Node> m_cameraLookNode;
    midp::ReferenceCountedPointer<AnimPlayer3D> m_cameraPosAnim;
    midp::ReferenceCountedPointer<AnimPlayer3D> m_cameraLookAnim;
    bool m_jerking;
    float m_jerkT;
    MathVector m_jerkFrom;
    MathVector m_jerkTo;
    float m_jerkFromYaw;
    float m_jerkFromPitch;
    float m_jerkToYaw;
    float m_jerkToPitch;
    bool m_titleReady;
    int m_launchStep;
    midp::SafeReferenceCountedPointer<Mayhem::GetChallenge> m_getChallenge;
    UI::Window* m_levelWindow;
    WrappedString m_introText;
    UI::SoundOptions* m_soundOptions;
    UI::Window* m_gameOptions;
    UI::AchievementWindow* m_achievements;
    UI::AboutMenu* m_aboutMenu;
    int m_openSubMenu;
    int m_openItem;
    int m_tickerIndex;
    int m_tickerGap;
    float m_tickerX;
    float m_tickerX2;
    midp::ReferenceCountedPointer<midp::String> m_tickerUrl;
    int m_mayhemStep;
    bool m_mayhemDone;
    bool m_mayhemFailed;
    midp::SafeReferenceCountedPointer<Mayhem::UserRequest> m_userRequest;
    UI::PendingChallengeWindow* m_pendingWindow;
};
