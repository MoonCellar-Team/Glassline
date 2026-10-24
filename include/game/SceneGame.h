#pragma once

#include <map>
#include <string>

#include "game/InterpolationTimed.h"
#include "game/MEdgeMap.h"
#include "game/Scene.h"
#include "game/SignalFilter.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundSequencer.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class AchievementData;
class GameObjectCheckpoint;
class GameObjectGhost;
class GameObjectPlayer;
class GameScreen;
namespace UI {
class PleaseWaitWindow;
class SoundOptions;
}
namespace Mayhem {
class PostChallenge;
class PostStatRequest;
class UserRequest;
}
namespace m3g {
class Group;
class Image2D;
class Node;
class World;
}
namespace midp {
class Graphics;
class Thread;
}

class SceneGame : public Scene {
public:
    enum GameState {
        STATE_NONE = 0,
        STATE_LOADING = 1,
        STATE_INTRO_CAM = 2,
        STATE_RACE_START = 3,
        STATE_GAME = 4,
        STATE_FADE_OUT = 5,
        STATE_LEVEL_COMPLETE_SINGLE = 6,
        STATE_LEVEL_COMPLETE_MULTIPLAYER = 7,
        STATE_MENU_PAUSED = 8,
        STATE_GAME_OPTIONS = 9,
        STATE_SOUND_OPTIONS = 10,
        STATE_POPUP_BOX = 11,
        STATE_CAMERA_ANIM = 12,
        STATE_HELP = 13,
        STATE_ABOUT = 14,
        STATE_RESTART_LEVEL = 15,
        STATE_NEXT_LEVEL = 16,
        STATE_EXIT_LEVEL = 17,
        STATE_FINISHED_GAME_STORY = 18,
        STATE_MEDIA_PICKER = 19,
        STATE_UPLOAD_CONFIRM = 20,
        STATE_DISPLAY_NAME_ENTRY = 21,
        STATE_MAYHEM_WAIT = 22,
        STATE_EXIT_FADE = 25,
        STATE_CHALLENGE_USER = 26,
        STATE_CHALLENGE_COMMENT = 27,
    };

    static const char* LAST_GHOST_ANIMATION_FILENAME;

    explicit SceneGame(AppEngine* engine);
    virtual ~SceneGame();

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

    int getState();
    void stateTransition(GameState state);
    void stateTransitionFade(GameState state);
    void setExitState(GameState state);
    void initState();
    void deinitState();

    void initGame();
    void updateGame(int dt);
    void renderGame(midp::Graphics* g);
    void initRaceStart();
    void updateRaceStart(int dt);
    void initMenuPaused();
    void updateMenuPaused(int dt);
    void deinitMenuPaused();
    void resumeMenuPaused();
#ifdef MEDECOMP_PORT
    bool isMenuOverGame();
    int getStateBehindMenu();
    void closeMenuOverGame();
    void updateBehindMenu(int dt);
#endif
    void initGameOptions();
    void updateGameOptions(int dt);
    void renderGameOptions(midp::Graphics* g);
    void initSoundOptions();
    void updateSoundOptions(int dt);
    void renderSoundOptions(midp::Graphics* g);
    void initHelp();
    void updateHelp(int dt);
    void renderHelp(midp::Graphics* g);
    void initAbout();
    void updateAbout(int dt);
    void renderAbout(midp::Graphics* g);
    void initMediaPicker();
    void updateMediaPicker();
    void initLevelCompleteSingle();
    void initLevelCompleteMultiplayer();
    void initFinishedGameStory();
    void updateFinishedGameStory(int dt);
    void renderFinishedGameStory(midp::Graphics* g);
    void deinitFinishedGameStory();
    void initMayhemWait();
    void updateMayhemWait(int dt);
    void triggerPopupBox(int window);
    void updatePopupBox(int dt);

    void updateLoading(int dt);
    void updateLoadingState(int dt);
    void renderLoading(midp::Graphics* g);
    void renderLoadingFirstPass(midp::Graphics* g);
    void extractUploadQueue(m3g::Node* node);
    void extractUploadQueueScoped(m3g::Node* node, int scope);
    void processUploadQueue();
    bool uploadQueueExhausted();
    void loadSounds();
    void unloadSounds();

    void resetLevel(GameState next);
    void userRestartingLevel();
    void completeLevel();
    void resetCheckpoint();
    void resetCheckPointsLevel();
    void checkpointReached();
    bool isCheckPointActive(GameObjectCheckpoint* checkpoint);
    void addPlayerObject(GameObjectPlayer* player);
    GameObjectPlayer* getSinglePlayerObject();
    GameScreen* getScreenForPlayer(GameObjectPlayer* player);
    int numGameScreens();
    GameScreen* getGameScreen(int index);
    void playerHurt(GameObjectPlayer* player, bool knockdown);
    void playCutscene(int cutscene);
    void startIntroCam();
    void startCombatTime();
    void updateRaceTime(int dt);
    int getRaceTime() const;
    void updateCamera(int dt);
    m3g::Group* getM3GWorld();
    MEdgeMap& getMap();

    void pointerPressedGame(int x, int y, int id);
    void pointerDraggedGame(int x, int y, int id);
    void pointerReleasedGame(int x, int y, int id);

    void updateAccelerometer(int dt);
    float getAccelerometerTilt();
    float getAccelerometerTiltVelocity() const;
    void setGameTimeFactor(float factor);
    void clearGameTimeFactor();
    float getGameTimeFactor();
    void updateSound(int dt);
    SoundSequencer& getSoundSequencer();
    SoundEventPoolManager& getSoundPoolManager();
    int getSoundEffectModifying();
    void setSoundEffectModifying(int value);
    void clearSoundEffectModifying();
    int getSoundMusicModifying();
    void setSoundMusicModifying(int value);
    void clearSoundMusicModifying();

    void startTopMessage(int text, int text2);
    void updateTopMessage(int dt);
    void renderTopMessage(midp::Graphics* g);

    void uploadChallenge();
    bool uploadSpeedrunStat(bool param);

    m3g::World* m_world;
    MEdgeMap m_map;
    int m_raceTime;
    int m_unk1c8;
    SignalFilter m_tiltFilter;
    float m_tiltVelocity;
    midp::array<midp::ReferenceCountedPointer<GameScreen> > m_gameScreens;
    GameState m_state;
    GameState m_fadeState;
    GameState m_prevState;
    GameState m_pausedState;
    int m_stateTime;
    bool m_gameActive;
    midp::ReferenceCountedPointer<midp::Thread> m_loadingThread;
    int m_loadingThreadState;
    int m_loadingStep;
    GameState m_stateAfterLoading;
    int m_soundEffectModifying;
    int m_soundMusicModifying;
    std::map<m3g::Image2D*, m3g::Image2D*> m_uploadQueue;
    UI::SoundOptions* m_soundOptionsWindow;
    SignalFilter m_sfxVolume;
    GameState m_popupReturnState;
    int m_finishedStoryTimer;
    int m_topMessageText;
    int m_topMessageText2;
    int m_topMessagePhase;
    InterpolationFloatTimed m_topMessageAnim;
    InterpolationFloatTimed m_topMessageAnim2;
    SoundSequencer m_soundSequencer;
    SoundEventPoolManager m_soundPoolManager;
    SignalFilter m_gameTimeFactor;
    int m_combatTime;
    AchievementData* m_achievementData;
    float m_loadingScrollY;
    bool m_loadingScrolled;
    midp::ReferenceCountedPointer<GameObjectGhost> m_ghost;
    int m_completedRaceTime;
    int m_prevBestTime;
    bool m_newRecord;
    bool m_firstCompletion;
    GameState m_exitState;
    std::string m_displayName;
    std::string m_challengeUserId;
    std::string m_challengeComment;
    UI::PleaseWaitWindow* m_pleaseWaitWindow;
    int m_mayhemStep;
    int m_mayhemPendingStep;
    midp::SafeReferenceCountedPointer<Mayhem::UserRequest> m_userRequest;
    midp::SafeReferenceCountedPointer<Mayhem::PostChallenge> m_postChallenge;
    midp::SafeReferenceCountedPointer<Mayhem::PostStatRequest> m_postStatRequest;
    int m_storyStartTimer;
    bool m_mayhemResetSession;
};
