#pragma once

#include "game/GameScreenPointer.h"
#include "game/MathFrustum.h"
#include "game/MenuPause.h"
#include "game/GameUI/SolidRect.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"

class GameCamera;
class GameObjectCheckpoint;
class GameObjectPlayer;
class QuadManager;
class SceneGame;
namespace GameUI {
class Confirmation;
class EndOfLevelResults;
class ScrollingText;
class SoundSettings;
}
namespace m3g {
class Camera;
class Graphics3D;
class World;
}
namespace midp {
class Graphics;
class String;
}

class GameScreen : public midp::ReferenceCounted {
public:
    enum ScreenType {
        TYPE_SINGLE = 0,
        TYPE_SPLIT_FIRST = 1,
        TYPE_SPLIT_SECOND = 2,
    };

    enum ScreenState {
        SCREEN_GAME = 0,
        SCREEN_RACE_START_3 = 1,
        SCREEN_RACE_START_2 = 2,
        SCREEN_RACE_START_1 = 3,
        SCREEN_DEATH_FADE = 4,
        SCREEN_MENU_MAIN = 5,
        SCREEN_SOUND_SETTINGS = 6,
        SCREEN_HELP_MENU = 7,
        SCREEN_MULTIPLAYER_COMPLETE = 8,
        SCREEN_CONFIRM_RESTART = 9,
        SCREEN_CONFIRM_QUIT = 10,
    };

    enum InputFlag {
        INPUT_SWIPE_LEFT = 0x01,
        INPUT_SWIPE_RIGHT = 0x02,
        INPUT_SWIPE_UP = 0x04,
        INPUT_SWIPE_DOWN = 0x08,
        INPUT_TAP = 0x10,
        INPUT_HOLD = 0x20,
        INPUT_SWIPE2_LEFT = 0x40,
        INPUT_SWIPE2_RIGHT = 0x80,
        INPUT_SWIPE2_UP = 0x100,
        INPUT_SWIPE2_DOWN = 0x200,
    };

    GameScreen(SceneGame* scene, ScreenType type, m3g::World* world);
    virtual ~GameScreen();

    int getState();
    void stateTransition(ScreenState state);
    bool isInGameState();
    bool isInRaceStart();
    void update(int dt);
    void menuPause();
    void menuResume();
    void resetLevel();

    void initRaceStart();
    void updateRaceStart(int dt);
    void initDeathFade();
    void updateDeathFade(int dt);
    void deinitDeathFade(ScreenState next);
    void initMenuMain();
    void updateMenuMain(int dt);
    void selectMenuMain(int item);
    void initMultiplayerComplete();
    void confirmResult(bool yes);

    GameCamera* getGameCamera();
    MathFrustum* getCameraFrustum();
    GameObjectPlayer* getPlayerObject();
    void setPlayerObject(GameObjectPlayer* player);
    GameObjectCheckpoint* getCheckpointObject();
    void setCheckpointObject(GameObjectCheckpoint* checkpoint);
    void incNumCollectablesCollected();
    void decNumCollectablesCollected();
    int getEffectScope();
    int getLocalWidth();
    int getLocalHeight();

    void updateCamera(int dt);
    void startCameraShake(float intensity, int duration);
    void updateEffectAnims(QuadManager& quads, int dt);
    void startPainEffect(QuadManager& quads);
    void startFlashEffect(QuadManager& quads);
    void fadeToBlack();
    void fadeFromBlack();

    void render(midp::Graphics* g);
    void renderState2D(midp::Graphics* g);
    void renderState3D(midp::Graphics* g, m3g::Graphics3D* g3d);
    void renderHud2D(midp::Graphics* g);
    void renderHud3D(m3g::Graphics3D* g3d);
    void renderInverseHud2D(midp::Graphics* g);
    void renderRaceStart2D(midp::Graphics* g);
    void renderMenuMain2D(midp::Graphics* g);
    void renderMenuMain3D(midp::Graphics* g, m3g::Graphics3D* g3d);
    void setupLocatorQuadForRender(GameObjectPlayer* player);
#ifdef MEDECOMP_PORT
    void setupLocatorQuad(const MathVector& at);
#endif
    void getScreenLocalPosition(const MathVector& pos, int& x, int& y) const;
    void getScreenLocalPositionNoZCheck(const MathVector& pos, int& x, int& y) const;
    void calculateVectorToScreenSide(int& x, int& y);
    void calculateVectorToScreenSidePositive(int& x, int& y);

    bool pointerPressed(int x, int y, int id, int canvasWidth, int canvasHeight);
    bool pointerDragged(int x, int y, int id, int canvasWidth, int canvasHeight);
    bool pointerReleased(int id);
    void updatePointers(int dt);
    void applyGestures();
    GameScreenPointer* getPointer(int id);
    bool calcLocalCoordinates(int x, int y, int& localX, int& localY);
    void calcGlobalCoordinates(int localX, int localY, int& x, int& y);
    void pointerReleasedGame(GameScreenPointer* pointer);
    void pointerReleasedSoundSettings(GameScreenPointer* pointer);
    void pointerReleasedHelpMenu(GameScreenPointer* pointer);
    void pointerReleasedMultiplayerComplete(GameScreenPointer* pointer);
    void pointerReleasedConfirmation(GameScreenPointer* pointer);

    SceneGame* m_scene;
    midp::ReferenceCountedPointer<m3g::World> m_world;
    ScreenType m_type;
    int m_width;
    int m_height;
    ScreenState m_state;
    ScreenState m_resumeState;
    midp::ReferenceCountedPointer<GameCamera> m_camera;
    midp::ReferenceCountedPointer<m3g::Camera> m_m3gCamera;
    MathFrustum m_frustum;
    midp::array<float> m_cameraPos;
    int m_inputFlags;
    GameScreenPointer m_pointer1;
    GameScreenPointer m_pointer2;
    midp::ReferenceCountedPointer<GameObjectPlayer> m_player;
    int m_effectScope;
    int m_fadeMesh;
    int m_fadeAnim;
    int m_damageMesh;
    int m_painAnim;
    int m_flashAnim;
    int m_collectMesh;
    int m_collectAnim;
    bool m_showInverseCount;
    int m_painSoundHandle;
    int m_collectHudTimer;
    GameObjectCheckpoint* m_checkpoint;
    GameUI::SolidRect m_dimRect;
    MenuPause m_menu;
    midp::ReferenceCountedPointer<GameUI::SoundSettings> m_soundSettings;
    midp::ReferenceCountedPointer<GameUI::EndOfLevelResults> m_results;
    midp::ReferenceCountedPointer<GameUI::ScrollingText> m_helpText;
    midp::ReferenceCountedPointer<GameUI::Confirmation> m_confirmation;
    int m_raceStartTimer;
    midp::ReferenceCountedPointer<midp::String> m_countdownText;
};
