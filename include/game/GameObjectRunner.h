#pragma once

#include "game/GameObject.h"

class AnimationBlender;
class AnimPlayer3D;

class GameObjectRunner : public GameObject {
public:
    enum FacingDir {
        FACING_LEFT = -1,
        FACING_RIGHT = 1,
    };

    enum VisualType {
        VISUAL_SOLO = 0,
        VISUAL_ORIGIN = 1,
        VISUAL_BLEND3 = 2,
    };

    static const short RESOURCE_ARRAY[];
    static const short ANIM_FLAG_ARRAY[];

    GameObjectRunner(MEdgeMap* map, int type);
    GameObjectRunner(MEdgeMap* map, int type, float x, float y, float z);
    virtual ~GameObjectRunner();

    virtual void update(int dt);
    virtual void updateAnimation(int dt);
    virtual void setVisual(int type, int index);
    virtual void setOffsetVisual(int index, float height);
    virtual void animationStopped();

    void setVisual(int visual);
    void setVisualAssets(int modelId, int blenderChannels);
    static int hashVisual(int type, int index, int facingDir);
    int getVisual();
    int getVisualType();
    int getVisualIndex();
    int getVisualDuration(int type, int index);
    bool isInOriginAnimation();

    FacingDir getFacingDir() const;
    void setFacingDir(FacingDir dir);
    void reverseFacingDir();

    float get3WayAnimationBlendWeights();
    void set3WayAnimationBlendWeights(float weight);
    float calcAnimProgress() const;

    void updateMovementOriginAnimation(int facingDir, int dt);
    void calculateOffsetAnimationStartOffset(int index, MathVector& out);
    void calculateOffsetAnimationOverallOffset(int index, MathVector& out);
    float getOriginAnimationYOffset(int index);
    MathVector getPreOriginAnimPosNoOffset();

    midp::ReferenceCountedPointer<m3g::Node> m_model;
    midp::ReferenceCountedPointer<AnimationBlender> m_blender;
    midp::ReferenceCountedPointer<m3g::Node> m_originNode;
    AnimPlayer3D* m_originAnimPlayer;
    float m_originStartY;
    float m_originYScale;
    float m_originHeight;
    int m_visualType;
    int m_visualIndex;
    float m_blendWeight;
    MathVector m_preOriginAnimPos;
    MathVector m_originBasePos;
    FacingDir m_facingDir;
};
