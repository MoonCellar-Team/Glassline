#pragma once

#include "game/ChunkManager.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/array.h"
#include "runtime/util/VectorSE.h"

class CollShape;
class DataCameraAnim;
class FallBox;
class GameObject;
class GameObjectCulledGraphic;
class GameObjectPlayer;
class MathFrustum;
class MathLine;
class MathVector;
class ModelSet;
class PainBox;
class Path;
namespace m3g {
class Group;
class Node;
class Texture2D;
class World;
}

class MEdgeMap {
public:
    enum {
        HIT_LEFT = 1,
        HIT_RIGHT = 2,
        HIT_FLOOR = 4,
        HIT_CEILING = 8,
        HIT_NEAR = 0x10,
        HIT_FAR = 0x20,
        HIT_FLOOR_SLOPE_LEFT = 0x40,
        HIT_FLOOR_SLOPE_RIGHT = 0x80,
        HIT_CEILING_SLOPE_LEFT = 0x100,
        HIT_CEILING_SLOPE_RIGHT = 0x200,
        HIT_SLOPES = 0x3c0,
    };

    enum {
        COLLIDES_NONE = 0,
        COLLIDES_WITH_PLAYERS = 1,
        COLLIDES_WITH_OBJECTS = 2,
    };

    static const int NUM_LAYERS = 4;

    static const int LEVEL_DATA_LOOKUP_ARRAY[];
    static const int RES_LOOKUP_ARRAY[];
    static const int MATERIAL_LOOKUP_ARRAY[];
    static const short MAP_OBJECT_IDS[];
    static const short MAP_STRING_IDS[];
    static const short MAP_CUTSCENE_IDS[];
    static float COLLISION_BOX_MIN;
    static float COLLISION_BOX_MAX;

    MEdgeMap();
    ~MEdgeMap();

    bool intersects(const MathLine& line, int layer, float& tMin, float& tMax);
    bool intersects(CollShape* shape, int layer);
    bool intersects(const MathVector& point, int layer);
    bool intersects(const MathVector& point, int layer, CollShape** hit);
    int moveObject(MathVector& position, MathVector& velocity, float dt, CollShape* shape);
    int moveObjectPhysics(MathVector& position, MathVector& velocity, float dt, CollShape* shape,
                          CollShape** hit, MathVector& normal);
    CollShape* getAttributeShapeAt(const MathLine& line, int layer);
    float calculateCollisionMoveOut(const MathLine& line, int layer);
    bool calculateCollision(const MathLine& line, float& distance, int layer);
    void getAttribSectionBounds(int layer, float min, float max, int& start, int& end);
    void getAttribSectionBounds(int layer, float x, int& start, int& end);
    void getAttribSectionBounds(int layer, const MathLine& line, float tMin, float tMax, int& start, int& end);
    int getAttribSectionOffset(int layer, float x);
    bool isInFallBox(GameObject* obj);
    bool isInPainBox(GameObject* obj);

    void addObject(GameObject* obj);
    void removeObject(GameObject* obj);
    void removeDynamicObject(GameObject* obj);
    int getNumPlayerCheckObjects();
    GameObject* getPlayerCheckObject(int index);
    GameObject* getGameObjectAt(const MathLine& line, GameObject* exclude);
    int getNumCollectablesTotal();
    util::VectorSE<GameObject>& getCollectableList();
    void updateCollectableList();
    void singlePlayerCheckpointActivated();
    int getUniqueGameObjectID(GameObject* obj);
    void resetLevel();
    void resetCheckpoint();
    void updateObjects(int dt);
    void updateObjectCollisions();

    void addCullObject(GameObjectCulledGraphic* graphic);
    void removeCullObject(GameObjectCulledGraphic* graphic);
    void updateChunkVisibility(int dt);
    void cullView(const MathFrustum& frustum);
    void updateObjectCulling(int screen);
    m3g::Node* getSkyDomeNode();

    void loadMap(int resId, m3g::World* world);
    void freeMap();
    int getModelId(int index);
    ModelSet* getModelSet();
    Path* getPath(int index);
    DataCameraAnim* getCutscene(int index);

    int m_nextObjectID;
    int m_unk004;
    ModelSet* m_modelSet;
    float m_minX;
    float m_maxX;
    float m_sectionWidth;
    int m_numSections;
    midp::array<short> m_sectionStart;
    midp::array<short> m_sectionEnd;
    midp::array<CollShape*> m_shapes;
    bool m_loading;
    union { util::VectorSE<GameObject> m_staticObjects; char m_staticObjectsBytes[0x18]; };
    union { util::VectorSE<GameObject> m_dynamicObjects; char m_dynamicObjectsBytes[0x18]; };
    union { util::VectorSE<GameObjectPlayer> m_players; char m_playersBytes[0x18]; };
    union { util::VectorSE<GameObject> m_collectables; char m_collectablesBytes[0x18]; };
    union { util::VectorSE<GameObject> m_updateObjects; char m_updateObjectsBytes[0x18]; };
    union { util::VectorSE<GameObject> m_playerCollisionObjects; char m_playerCollisionObjectsBytes[0x18]; };
    union { util::VectorSE<GameObject> m_objectCollisionObjects; char m_objectCollisionObjectsBytes[0x18]; };
    union { util::VectorSE<GameObject> m_collisionObjects; char m_collisionObjectsBytes[0x18]; };
    union { util::VectorSE<GameObjectCulledGraphic> m_cullObjects; char m_cullObjectsBytes[0x18]; };
    midp::ReferenceCountedPointer<m3g::Group> m_mapGroup;
    midp::ReferenceCountedPointer<m3g::Node> m_skyDome;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_scrollTexture1;
    midp::ReferenceCountedPointer<m3g::Texture2D> m_scrollTexture2;
    float m_scrollOffset1;
    float m_scrollOffset2;
    ChunkManager m_chunkManager;
    midp::array<midp::ReferenceCountedPointer<DataCameraAnim> > m_cutscenes;
    midp::array<Path*> m_paths;
    midp::array<FallBox*> m_fallBoxes;
    midp::array<PainBox*> m_painBoxes;
};
