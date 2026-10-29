#pragma once

#ifdef MEDECOMP_PORT
#include "port/wall_climb.h"
#endif

#include "game/GameObjectRunner.h"
#include "game/InterpolationTimed.h"
#include "game/SignalFilter.h"

class AchievementData;
class CollShape;
class GameObjectNPC;
class GameObjectZipLine;
class GhostAnimationRecorder;
class GhostKeyframe;
namespace particles {
class ParticleEffect;
}

class GameObjectPlayer : public GameObjectRunner {
public:
    enum PlayerState {
        STATE_NONE = 0,
        STATE_START = 1,
        STATE_IDLE = 2,
        STATE_RUNNING = 3,
        STATE_RUNNING_TURNING = 4,
        STATE_BALANCE_RUNNING = 5,
        STATE_BALANCE_FELL = 6,
        STATE_STOPPING = 7,
        STATE_FALLING_VERTICALLY = 8,
        STATE_FALLING_FORWARDS = 9,
        STATE_10 = 10,
        STATE_ROLLING = 11,
        STATE_CRASH = 12,
        STATE_WALL_CLIMB = 13,
        STATE_CLAMBER = 14,
        STATE_HANG = 15,
        STATE_HANG_CLAMBER = 16,
        STATE_SLIDING = 17,
        STATE_WALL_RUN = 18,
        STATE_WALL_SLIDE = 19,
        STATE_SWINGING = 20,
        STATE_SCALE_JUMP = 21,
        STATE_SCALE_HAUL_UP = 22,
        STATE_RAMP_RUN_UP = 23,
        STATE_RAMP_SLIDE = 24,
        STATE_ZIPLINE_JUMP_TO = 25,
        STATE_ZIPLINE = 26,
        STATE_ZIPLINE_FAIL = 27,
        STATE_MELEED_IN_THE_FACE = 28,
        STATE_DISARMING_COP = 29,
        STATE_LYING = 30,
        STATE_GETTING_UP = 31,
        STATE_GETUP_MELEE = 32,
        STATE_FLYKICK = 33,
        STATE_DYING = 34,
        STATE_DEAD = 35,
    };

    enum AnimationState {
        ANIM_NONE = 0,
        ANIM_LANDING = 1,
        ANIM_VAULT_JUMP = 2,
        ANIM_BOOST_JUMP = 3,
        ANIM_BOOST_JUMP_END = 4,
        ANIM_HAUL_UP_TO_RUN = 5,
        ANIM_DROP_TO_WALL_SLIDE = 6,
        ANIM_WALL_KICK = 7,
        ANIM_STATE_8 = 8,
    };

    enum Gesture {
        GESTURE_LEFT = 0x01,
        GESTURE_LEFT_TWO_FINGER = 0x02,
        GESTURE_RIGHT = 0x04,
        GESTURE_RIGHT_TWO_FINGER = 0x08,
        GESTURE_STOP = 0x10,
        GESTURE_DOWN = 0x20,
        GESTURE_JUMP = 0x40,
        GESTURE_UP = 0x80,
        GESTURE_ANY_LEFT = GESTURE_LEFT | GESTURE_LEFT_TWO_FINGER,
        GESTURE_ANY_RIGHT = GESTURE_RIGHT | GESTURE_RIGHT_TWO_FINGER,
        GESTURE_ANY_SIDE = GESTURE_ANY_LEFT | GESTURE_ANY_RIGHT,
    };

    enum WallAction {
        WALL_CLAMBER_EXACT = 0x01,
        WALL_CLAMBER_LOW = 0x02,
        WALL_CLAMBER_PREDICT = 0x04,
        WALL_HANG = 0x08,
        WALL_CLIMB_SLIDE = 0x10,
    };

    GameObjectPlayer(MEdgeMap* map, int type, int playerIndex, float x, float y, float z,
                     FacingDir facingDir);
    virtual ~GameObjectPlayer();

    virtual void checkpointActivated();
    virtual void resetCheckpoint();
    virtual void resetLevel();
    virtual void collidedWith(GameObject* other);
    virtual void update(int dt);
    virtual void animationStopped();

    void resetCommon();
    void playerFail();
    void die();
    void hurt(float damage, bool knockdown);
    void updateHealth(int dt);
    float getHealthFraction();
    void setVictorious();
    bool isVictorious();

    int getPlayerIndex();
    PlayerState getState();
    float getForwardSpeed() const;
    void setForwardSpeed(float speed);
    void setZOffset(float offset);
    float getAccelerometerTilt(bool withNoise);
    void updateAccelerometerNoise(int dt);
    int getCurrentMaterial();
    GhostAnimationRecorder* getGhostAnimation();
    void updateGhost(int dt);
    void updateSound(int dt);
    void setupSoundSequence();

    int getNumCollectablesCollected();
    void incNumCollectablesCollected();
    void decNumCollectablesCollected();
    void resetNumCollectablesCollected();

    bool isAttacking() const;
    static bool isCrouchState(PlayerState state);
    bool isDead();
    bool isDisarming() const;
    bool isGettingUp() const;
    bool isGetupMeleeing() const;
    bool isImmuneToOtherPlayer();
    bool isKicking() const;
    bool isLying() const;
    bool isMeleed() const;
    bool isOnRamp() const;
    bool isProne() const;
    bool isRolling() const;
    bool isSliding() const;

    void stateTransition(PlayerState state);
    void updateState(int dt);

    void initStateIdle(PlayerState prev);
    void initStateRunning(PlayerState prev);
    void initStateRunningTurning(PlayerState prev);
    void initStateBalanceRunning(PlayerState prev);
    void initStateBalanceFell(PlayerState prev);
    void initStateStopping(PlayerState prev);
    void initStateFalling(PlayerState prev);
    void initStateFallingVertically(PlayerState prev);
    void initStateRolling(PlayerState prev);
    void initStateCrash(PlayerState prev);
    void initStateWallClimb(PlayerState prev);
    void initStateClamber(PlayerState prev);
    void initStateHang(PlayerState prev);
    void initStateHangClamber(PlayerState prev);
    void initStateSliding(PlayerState prev);
    void initStateWallSlide(PlayerState prev);
    void initStateSwinging(PlayerState prev);
    void initStateScaleJump(PlayerState prev);
    void initStateRampRunUp(PlayerState prev);
    void initStateRampSlide(PlayerState prev);
    void initStateZipLineJumpTo(PlayerState prev);
    void initStateZipLine(PlayerState prev);
    void initStateZipLineFail(PlayerState prev);
    void initStateMeleedInTheFace(PlayerState prev);
    void initStateDisarmingCop(PlayerState prev);
    void initStateFlykick(PlayerState prev);

    void deinitStateRunning();
    void deinitStateBalanceFell();
    void deinitStateFallingVertically();
    void deinitStateFallingForwards();
    void deinitStateCrash();
    void deinitStateWallClimb();
    void deinitStateClamber();
    void deinitStateHang();
    void deinitStateSliding();
    void deinitStateWallRun();
    void deinitStateWallSlide();
    void deinitStateSwinging();
    void deinitStateScale();
    void deinitStateRampAny();
    void deinitStateZipLineJumpTo();
    void deinitStateZipLine();
    void deinitStateFlykick();

    void updateStateRunning(int dt);
    void updateStateRunningTurning(int dt);
    void updateStateBalanceRunning(int dt);
    void updateStateBalanceFell(int dt);
    void updateStateStopping(int dt);
    void updateStateFallingVertically(int dt);
    void updateStateFallingForwards(int dt);
    void updateStateRolling(int dt);
    void updateStateCrash(int dt);
    void updateStateWallClimb(int dt);
    void updateStateSliding(int dt);
    void updateStateWallRun(int dt);
    void updateStateWallSlide(int dt);
    void updateStateSwinging(int dt);
    void updateStateScaleJump(int dt);
    void updateStateScaleHaulUp(int dt);
    void updateStateRampRunUp(int dt);
    void updateStateRampSlide(int dt);
    void updateStateZipLineJumpTo(int dt);
    void updateStateZipLine(int dt);
    void updateStateZipLineFail(int dt);
    void updateStateMeleedInTheFace(int dt);
    void updateStateDisarmingCop(int dt);
    void updateStateFlykick(int dt);

    void animStoppedStateClamber();
    void animStoppedStateHangClamber();
    void animStoppedStateScaleHaulUp();

    void animStateTransition(AnimationState state);
    void setupAnimation();
    void refreshAnimation();
    void updateAnimationStateRunning();

    void addGesture(int gesture);
    void clearGesture(int gesture);
    void clearAllGestures();
    void clearAllGesturesExcept(int gestures);
    void clearInvalidGestures();
    bool isAnyGestureSet(int gestures) const;
    int getTapGesture();
    void applyGestures();
    void applyGestureUp();
    void applyGestureDown();
    void applyGestureSide();
    void applyGestureJump();
    void applyGestureStop();

    void playerJump(bool vault, bool jumpVariant);
    void playerSwingLaunch();
    void playerZipLine(GameObjectZipLine* zipLine);
    void playerClimbLedgeAfterCheck(FacingDir dir, int visual, int nextVisual, float exitSpeed);
    void playerFullClamber(FacingDir dir, CollShape* shape);
    void playerHalfClamber(FacingDir dir, CollShape* shape);
    void kickSomething();
    void meleeByPlayer();
    void meleeByCop(GameObjectNPC* cop);
    void disarmCop(GameObjectNPC* cop);
    void groundPlayer();
    void fallPlayer();
    void hitHead();
    void hitWall();
    void snapPlayerToGround();

    bool checkDeathFall();
    bool checkJumpToZipLine();
    bool checkLedgeScale();
    void checkLedgeScale_CalculateJumpAndScale(const MathVector& ledge, MathVector& jump, MathVector& scale);
    bool checkWallAction(FacingDir dir, bool wallClimb, int flags);
    bool checkWallrun();

    void updateVelocity(int dt);
    void updateMovement(int dt);
    void updateMovementPhysics(float seconds);
    void updateMovementNoClip(float seconds);
    void updateMovementLinearInterpolation(int dt);
    void startPositionInterpolation(const MathVector& from, const MathVector& to, int duration,
                                    InterpolationTimed::InterpolationType type, bool collide);
    void startPositionLinearInterpolation(const MathVector& from, const MathVector& to, int duration,
                                          bool collide);
    void completePositionInterpolation();
    void initLineMovement(const MathVector& direction, float speed, bool collide);
    void updateLineMovement(float seconds);
    void deinitLineMovement();

    void loadStateZipLineParticleEffect();
    void unloadStateZipLineParticleEffect();
    void loadParticleEffectsForRampAndWallSlides();
    void updateParticleEffectForRampAndWallSlides(int dt, float rate, float speed, particles::ParticleEffect* effect,
                                                  m3g::Node* node, float offsetX, float offsetY);

    MathVector m_lastPosition;
    int m_playerIndex;
    float m_swingAmplitude;
    float m_swingPhase;
    float m_swingPhaseOffset;
    float m_swingAngle;
    int m_swingDir;
    GameObject* m_lastCollidedObject;
    int m_collisionCooldown;
    bool m_victorious;
    SignalFilter m_tiltNoiseFilter;
    int m_accelerometerNoiseTimer;
    float m_zOffset;
    SignalFilter m_zOffsetFilter;
    FacingDir m_startFacingDir;
    MathVector m_checkpointPosition;
    FacingDir m_checkpointFacingDir;
    PlayerState m_state;
    int m_stateTime;
    int m_disarmTimer;
    int m_disarmCooldown;
    midp::ReferenceCountedPointer<CollShape> m_shapeNormal;
    midp::ReferenceCountedPointer<CollShape> m_shapeCrouch;
    midp::ReferenceCountedPointer<CollShape> m_shapeBox;
    int m_numCollectables;
    int m_gestures;
    int m_landingRollTimer;
    AnimationState m_animState;
    PlayerState m_animSetupState;
    bool m_animDirty;
    GhostAnimationRecorder* m_ghostRecorder;
    GhostKeyframe* m_ghostKeyframe;
    int m_ghostTime;
    int m_healthRegenDelay;
    int m_soundState;
    int m_soundAnimState;
    int m_climbSoundSequence;
    int m_runSoundSequence;
    bool m_crashed;
    float m_maxSpeed;
    float m_acceleration;
    int m_runningTimer;
    InterpolationFloatTimed m_speedInterpolation;
    InterpolationVectorTimed m_positionInterpolation;
    bool m_positionInterpCollide;
    bool m_skipMovementOnce;
    bool m_noClipMovement;
    bool m_lineMovement;
    float m_lineSpeed;
    MathVector m_lineDirection;
    int m_lineMovementMode;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_zipLineEffect;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_rampSlideEffect0;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_rampSlideEffect1;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_wallSlideEffect0;
    midp::ReferenceCountedPointer<particles::ParticleEffect> m_wallSlideEffect1;
    midp::ReferenceCountedPointer<m3g::Node> m_effectNode106;
    midp::ReferenceCountedPointer<m3g::Node> m_effectNode105;
    midp::ReferenceCountedPointer<m3g::Node> m_effectNode110;
    int m_immunityTimer;
    int m_clamberVisual;
    int m_clamberNextVisual;
    float m_clamberExitSpeed;
    float m_clamberOffsetHeight;
    int m_ledgeGrabBlock;
    int m_wallSlideBlock;
    bool m_jumpVariant;
    int m_clamberJumpWindow;
    bool m_rollIntoSlide;
    bool m_slideUnderObstacle;
    int m_wallRunCooldown;
    int m_scaleVisual;
    PlayerState m_scaleNextState;
    AnimationState m_scaleAnimState;
    CollShape* m_rampShape;
    GameObjectZipLine* m_zipLine;
    float m_zipLineEndOffset;
    MathVector m_zipLineTarget;
    int m_material;
    float m_health;
    AchievementData* m_achievementData;
    int m_lyingTimer;
#ifdef MEDECOMP_PORT
    port::WallClimbJump m_wallClimbJump;
#endif
};
