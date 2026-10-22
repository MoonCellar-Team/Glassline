#pragma once

#include <map>
#include <queue>
#include <string>

#include "ea_online/PendingChallenge.h"
#include "game/AchievementData.h"
#include "game/AnimPlayer.h"
#include "game/AnimationBlenderData.h"
#include "game/AnimationManager3D.h"
#include "game/AnimationManagerData.h"
#include "game/GameObjectRunnerData.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/PhotoPicker.h"
#include "game/QuadManager.h"
#include "game/ResourceManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "game/UI/WindowStore.h"
#include "m3g/OpaqueObject.h"
#include "runtime/midp/Displayable.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"
#include "runtime/midp/array.h"
#include "runtime/util/JRandom.h"
#include "runtime/util/VectorSE.h"

class Achievement;
class AppEngineThread;
class BGMusic;
class MediaPicker;
class MonkeyApp;
class ReflectionMapShader;
class Scene;
class SceneGame;
class SceneMenu;
namespace Mayhem {
class Token;
}
namespace UI {
class AchievementNotification;
class LoadingScreen;
class WindowStore;
}
namespace m3g {
class Appearance;
class Camera;
class Graphics3D;
class Node;
class Texture2D;
class Transform;
class World;
}
namespace midp {
class Graphics;
class MIDlet;
class String;
class StringBuffer;
class Thread;
}

class GameObjectData {
public:
    int getFlags(int type);
    void loadData();
    GameObjectData();
    ~GameObjectData();
    int getCollidesWith(int type);

    midp::array<unsigned short> m_flags;
    midp::array<unsigned char> m_collidesWith;
};

class AppEngine : public midp::Canvas {
public:
    enum StatType {
        STAT_COUNT = 1,
        STAT_OF = 2,
        STAT_TIME = 3,
        STAT_TIME_OR_EMPTY = 4,
    };

    static const char* PENDING_CHALLENGES_FILENAME;

    explicit AppEngine(MonkeyApp* midlet);
    virtual ~AppEngine();
    virtual void paint(midp::Graphics* g);
    virtual void showNotify();
    virtual void hideNotify();
    virtual void keyPressed(int keyCode);
    virtual void keyReleased(int keyCode);
    virtual void pointerDragged(int x, int y, int pointerId);
    virtual void pointerPressed(int x, int y, int pointerId, int tapCount);
    virtual void pointerReleased(int x, int y, int pointerId);

    static AppEngine* getCanvas();
    static GameObjectData* getGameObjectData();
    static GameObjectRunnerData* getGameObjectRunnerData();
    static AchievementData* getAchievementData();
    static LevelData* getLevelData();
    static M3GAssets* getM3GAssets();
    static AnimationBlenderData* getAnimationBlenderData();

    SceneGame* getSceneGame();
    SceneMenu* getSceneMenu();
    AnimationManager3D& getAnimationManager3D();
    AnimationManagerData& getAnimManData();
    SoundManager& getSoundManager();
    TextManager* getTextManager();
    bool storageFull() const;
    bool getTutorialBoxes() const;
    void setTutorialBoxes(bool show);
    void setUploadScores(bool upload);
    float randFloat(float min, float max);
    int rand(int min, int max);
    int randPercent();
    float randPercentile();
    int rand(int n);
    QuadManager& getQuadManager();
    m3g::Graphics3D* getGraphics3D();
    ReflectionMapShader* getReflectionShader() const;
    ResourceManager& getResourceManager();
    UI::WindowStore& getWindowStore();
    midp::MIDlet* getMIDlet();
    BGMusic* getBGMusic();
    MediaPicker* getMediaPicker();
    PhotoPicker* getPhotoPicker();
    bool saveRMSAppSettings();
    void saveGameOptions();

    void changeScene(int scene);
    void changeScene(int scene, int param);

    void startFadeOut(bool);
    void startFadeOut(bool, int color);
    void startFadeIn(bool);
    bool isFadedIn();
    bool isFading();
    bool isFadedOut();
    void setFadeColor(int color);
    void renderBgFade(midp::Graphics* g);

    void initLoadingScreen(int screen);
    void deinitLoadingScreen();
    void updateLoading(int dt);
    void renderLoading(midp::Graphics* g);
    float getLoadingScrollValue() const;
    void setLoadingScrollValue(float value);
    void setScrollingLoad(bool scrolling);

    void drawStatString(midp::Graphics* g, int font, int textId, StatType type, int value, int x, int y,
                        int anchor);
    void drawOfStatString(midp::Graphics* g, int font, int textId, int value, int total, int x, int y,
                          int anchor);

    bool IsMayhemSessionValid() const;
    int timeToMayhemScore(int timeMs, int levelIndex);

    void notifyAchievementComplete(Achievement* achievement);

    static void createAppEngine(MonkeyApp* midlet);

    void start();
    void end();
    void startThread();
    void pauseGame();
    void resumeGame();
    bool isPaused();
    void runLoop(int dt);
    void receivedNotification(std::map<std::string, std::string> info);

    void doStartupLoading();
    void loadLoadingAssets();
    void loadSounds();
    AnimPlayer* getLoadingRunAnimPlayer();
    bool launchedForChallenge();
    void setStorageFull(bool full);
    void endGame();
    void freeLoadingScreens();
    void loadLoadingScreens();
    void initAboutString();
    int getSaveFileError();
    void setChallengeTime(int timeMs);
    int getChallengeTime() const;
    const std::string& getChallengeID() const;
    void setChallengeID(const std::string& id);
    int getPendingChallengeCount() const;
    PendingChallenge getNextPendingChallenge();
    void popPendingChallenge();
    void clearPendingChallenges();
    int mayhemScoreToTime(int score, int levelIndex);
    void resetGameOptions();
    void loadGameOptions();
    void loadRMSAppSettings();
    void resetRMSAppSettings();
    void loadAnimManager3D();
    void savePendingChallenges();
    void loadPendingChallenges();
    void update(int dt);
    void stopThread();
    void stopFade();
    void performChangeScene(int scene, int param);
    UI::LoadingScreen* getLoadingScreen(int index);
    midp::StringBuffer* getStatLabel(int textId);
    void appendStatValue(midp::StringBuffer* buffer, StatType type, int value);
    void processPointerEvents();
    void addPointerEvent(int type, int x, int y, int id, int tapCount);

    int m_unk14;
    m3g::OpaqueObject<util::VectorSE<midp::Object>, 0x18> m_unk18;
    bool m_saveFileError;
    AppEngineThread* m_thread;
    bool m_painting;
    bool m_updating;
    MonkeyApp* m_midlet;
    bool m_running;
    midp::ReferenceCountedPointer<Scene> m_scene;
    bool m_paused;
    m3g::Graphics3D* m_graphics3d;
    ResourceManager m_resources;
    AnimationManagerData m_animManData;
    AnimationManager3D m_animManager3D;
    TextManager m_text;
    QuadManager m_quads;
    SoundManager m_sound;
    BGMusic* m_bgMusic;
    midp::ReferenceCountedPointer<midp::Thread> m_bgThread;
    util::JRandom m_random;
    bool m_unk408;
    bool m_startupDone;
    int m_pointerBuffer;
    midp::array<int> m_pointerCounts;
    midp::array<midp::array<int>> m_pointerTypes;
    midp::array<midp::array<int>> m_pointerX;
    midp::array<midp::array<int>> m_pointerY;
    midp::array<midp::array<int>> m_pointerIds;
    midp::array<midp::array<int>> m_pointerTaps;
    M3GAssets m_m3gAssets;
    GameObjectData m_gameObjectData;
    LevelData m_levelData;
    AnimationBlenderData m_animBlenderData;
    GameObjectRunnerData m_runnerData;
    AchievementData m_achievementData;
    bool m_fadeDone;
    int m_sceneId;
    int m_nextScene;
    int m_nextSceneParam;
    int m_unk7e4;
    int m_unk7e8;
    midp::ReferenceCountedPointer<midp::StringBuffer> m_stringBuffer;
    bool m_unk7f0;
    midp::String* m_osVersion;
    bool m_firstLaunch;
    AnimPlayer m_loadingRun;
    m3g::World* m_loadingWorld;
    m3g::Camera* m_loadingCamera;
    midp::ReferenceCountedPointer<m3g::Transform> m_loadingTransform;
    midp::ReferenceCountedPointer<m3g::Node> m_loadingNode;
    midp::ReferenceCountedPointer<m3g::Appearance> m_loadingAppearance;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_loadingTexture;
    float m_loadingScroll;
    bool m_scrollingLoad;
    midp::array<UI::LoadingScreen*> m_loadingScreens;
    MediaPicker* m_mediaPicker;
    UI::WindowStore m_windows;
    UI::AchievementNotification* m_achievementNote;
    midp::ReferenceCountedPointer<ReflectionMapShader> m_reflection;
    bool m_tutorialBoxes;
    bool m_uploadScores;
    bool m_unk8d6;
    bool m_spywareEnabled;
    PhotoPicker m_photoPicker;
    std::string m_mayhemId;
    midp::SafeReferenceCountedPointer<Mayhem::Token> m_mayhemToken;
    int m_challengeTime;
    bool m_launchedForChallenge;
    std::string m_challengeId;
    std::string m_unk91c;
    std::queue<PendingChallenge> m_pendingChallenges;
    bool m_storageFull;
};
