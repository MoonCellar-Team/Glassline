#include "game/MEdgeMap.h"

#include "ea_online/SpywareManager.h"
#include "game/AppEngine.h"
#include "game/ChunkRunnerVision.h"
#include "game/CollShape.h"
#include "game/DataCameraAnim.h"
#include "game/FallBox.h"
#include "game/GameCommon.h"
#include "game/GameObject.h"
#include "game/GameObjectAmbientSound.h"
#include "game/GameObjectCheckpoint.h"
#include "game/GameObjectChopper.h"
#include "game/GameObjectCollectable.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectCutsceneTrigger.h"
#include "game/GameObjectEffectPigeonTakeoff.h"
#include "game/GameObjectFinish.h"
#include "game/GameObjectMusicCue.h"
#include "game/GameObjectPlaneLight.h"
#include "game/GameObjectPlayer.h"
#include "game/GameObjectPoliceLight.h"
#include "game/GameObjectPoliceRiot.h"
#include "game/GameObjectPopupBox.h"
#include "game/GameObjectRivalBoss.h"
#include "game/GameObjectRivalGeneral.h"
#include "game/GameObjectSoundOneShot.h"
#include "game/GameObjectZipLine.h"
#include "game/GameScreen.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MapPalette.h"
#include "game/MathLine.h"
#include "game/MathOrthoBox.h"
#include "game/MathVector.h"
#include "game/ModelSet.h"
#include "game/PainBox.h"
#include "game/Path.h"
#include "game/ResourceManager.h"
#include "game/SceneGame.h"
#include "m3g/Appearance.h"
#include "m3g/Node.h"
#include "m3g/Texture2D.h"
#include "m3g/World.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/Math.h"
#ifdef MEDECOMP_PORT
#include <math.h>
#include <stdio.h>

#include "port/net.h"
#endif

static const int FLAG_PLAYER = 1;
static const int FLAG_COLLECTABLE = 8;
static const int FLAG_UPDATE = 16;

static const int LAYER_SOLID = 0;

static const float HIT_EPSILON = 0.01f;

static const float SLOPE_LIFT = 0.01f;

static const float FIXED_TO_FLOAT = 1.0f / 65536.0f;

static const float SCROLL_SPEED_1 = -0.0004f;
static const float SCROLL_SPEED_2 = -0.002f;

static const int NODE_SKY_DOME = 150;
static const int APPEARANCE_SCROLL_1 = 111;
static const int APPEARANCE_SCROLL_2 = 112;

enum MapObjectType {
    MAP_OBJECT_PIGEONS = 0,
    MAP_OBJECT_CHOPPER = 1,
    MAP_OBJECT_POLICE_LIGHT = 4,
    MAP_OBJECT_RIVAL_BOSS = 5,
    MAP_OBJECT_PLANE_LIGHT = 6,
    MAP_OBJECT_RIVAL_GENERAL = 7,
    MAP_OBJECT_POLICE_RIOT = 8,
};

const int MEdgeMap::LEVEL_DATA_LOOKUP_ARRAY[] = {0, 1, 3, 2, 4, 5, 8, 9, 7, 6, 0, 0};

const int MEdgeMap::RES_LOOKUP_ARRAY[] = {
    34, 35,  6, 67, 66, 55, 13,  7, 69,  9, 21,  8,
    22, 12, 78,  9, 10, 68, 26, 11, 28, 12, 33, 13,
    14, 15, 15, 19, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33,  0,  0,
};

const int MEdgeMap::MATERIAL_LOOKUP_ARRAY[] = {0, 2, 3, 4, 1, 5, 0, 0};

const short MEdgeMap::MAP_OBJECT_IDS[] = {22, 7, 18, 19, 3, 6, 8, 5, 4, 0, 0, 0, 0, 0, 0, 0};

const short MEdgeMap::MAP_STRING_IDS[] = {
    2118, 2141, 2115, 2117, 2129, 2119, 2126, 2120, 2148, 2143,
    2138, 2127, 2128, 2146, 2139, 2136, 2125, 2121, 2124, 2122,
    2131, 2132, 2130, 2149, 2134, 2150, 2151, 2133, 2140, 2135,
    2156, 2154, 2155, 2157, 2220, 2152, 2153, 2218, 2238, 2334,
};

const short MEdgeMap::MAP_CUTSCENE_IDS[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#ifdef MEDECOMP_PORT
static void snapFloorTops(midp::array<CollShape*>& shapes) {
    const float kStep = 0.001f;
    int count = shapes.m_length;
    for (int i = 0; i != count; i++) {
        CollShape* shape = shapes[i];
        if (shape == NULL || shape->getShapeType() != CollShape::SHAPE_ORTHO_HEXAHEDRON) continue;
        MathOrthoBox box = shape->getLocalBounds();
        if (box.m_max.x - box.m_min.x <= kStep) continue;
        float top = box.m_max.y;
        for (int j = 0; j != count; j++) {
            CollShape* other = shapes[j];
            if (j == i || other == NULL || other->getShapeType() != CollShape::SHAPE_ORTHO_HEXAHEDRON) continue;
            const MathOrthoBox& near = other->getLocalBounds();
            if (near.m_max.x - near.m_min.x <= kStep) continue;
            bool touching = fabsf(box.m_max.x - near.m_min.x) < kStep || fabsf(near.m_max.x - box.m_min.x) < kStep;
            float above = box.m_max.y - near.m_max.y;
            if (touching && above > 0.0f && above < kStep && near.m_max.y < top) top = near.m_max.y;
        }
        if (top == box.m_max.y) continue;
        CollShape* snapped = new CollOrthoHexahedron(box.m_min.x, box.m_min.y, box.m_min.z, box.m_max.x, top, box.m_max.z);
        snapped->m_material = shape->m_material;
        midp::INCREF(snapped);
        midp::DECREF(shape);
        shapes[i] = snapped;
        fprintf(stderr, "medecomp: карта: форма %d — верх %.6f -> %.6f, как у соседней (K11)\n", i, box.m_max.y, top);
    }
}

extern "C" midp::InputStream* medecomp_player_map(void);
extern "C" int medecomp_player_level_playing(void);
void medecomp_level_supports(MEdgeMap* map, m3g::Group* group, MapPalette* palette);
extern "C" int medecomp_map_table_size(int table) {
    switch (table) {
    case 0: return sizeof(MEdgeMap::RES_LOOKUP_ARRAY) / sizeof(MEdgeMap::RES_LOOKUP_ARRAY[0]);
    case 1: return sizeof(MEdgeMap::LEVEL_DATA_LOOKUP_ARRAY) / sizeof(MEdgeMap::LEVEL_DATA_LOOKUP_ARRAY[0]);
    case 2: return sizeof(MEdgeMap::MATERIAL_LOOKUP_ARRAY) / sizeof(MEdgeMap::MATERIAL_LOOKUP_ARRAY[0]);
    case 3: return sizeof(MEdgeMap::MAP_OBJECT_IDS) / sizeof(MEdgeMap::MAP_OBJECT_IDS[0]);
    case 4: return sizeof(MEdgeMap::MAP_STRING_IDS) / sizeof(MEdgeMap::MAP_STRING_IDS[0]);
    case 5: return sizeof(MEdgeMap::MAP_CUTSCENE_IDS) / sizeof(MEdgeMap::MAP_CUTSCENE_IDS[0]);
    }
    return 0;
}
#endif

float MEdgeMap::COLLISION_BOX_MIN = -9.0f;
float MEdgeMap::COLLISION_BOX_MAX = 1.0f;

static float readFixed(midp::DataInputStream& in) {
    return (float)in.readInt() * FIXED_TO_FLOAT;
}

static int hitSides(const MathVector& normal) {
    int result = 0;
    if (normal.x > HIT_EPSILON) {
        result |= MEdgeMap::HIT_LEFT;
    } else if (normal.x < -HIT_EPSILON) {
        result |= MEdgeMap::HIT_RIGHT;
    }
    if (normal.y > HIT_EPSILON) {
        result |= MEdgeMap::HIT_FLOOR;
    } else if (normal.y < -HIT_EPSILON) {
        result |= MEdgeMap::HIT_CEILING;
    }
    if (normal.z > HIT_EPSILON) {
        result |= MEdgeMap::HIT_NEAR;
    } else if (normal.z < -HIT_EPSILON) {
        result |= MEdgeMap::HIT_FAR;
    }
    return result;
}

ModelSet* MEdgeMap::getModelSet() {
    return m_modelSet;
}

util::VectorSE<GameObject>& MEdgeMap::getCollectableList() {
    return m_collectables;
}

int MEdgeMap::getNumCollectablesTotal() {
    return m_collectables.size();
}

DataCameraAnim* MEdgeMap::getCutscene(int index) {
    return m_cutscenes[index];
}

void MEdgeMap::cullView(const MathFrustum& frustum) {
    m_chunkManager.cullView(frustum);
}

void MEdgeMap::getAttribSectionBounds(int layer, float min, float max, int& start, int& end) {
    if (m_minX > max || m_maxX < min) {
        start = 0;
        end = 0;
        return;
    }
    start = m_sectionStart[getAttribSectionOffset(layer, min)];
    end = m_sectionEnd[getAttribSectionOffset(layer, max)];
}

void MEdgeMap::getAttribSectionBounds(int layer, float x, int& start, int& end) {
    if (m_minX > x || m_maxX < x) {
        start = 0;
        end = 0;
        return;
    }
    int offset = getAttribSectionOffset(layer, x);
    start = m_sectionStart[offset];
    end = m_sectionEnd[offset];
}

void MEdgeMap::getAttribSectionBounds(int layer, const MathLine& line, float tMin, float tMax, int& start,
                                      int& end) {
    float x = line.m_origin.x;
    float dx = line.m_direction.x;
    if (dx == 0.0f) {
        getAttribSectionBounds(layer, x, start, end);
        return;
    }
    float x1 = x + dx * tMin;
    float x2 = x + dx * tMax;
    float min = midp::Math::min(x1, x2);
    float max = midp::Math::max(x1, x2);
    getAttribSectionBounds(layer, min, max, start, end);
}

GameObject* MEdgeMap::getGameObjectAt(const MathLine& line, GameObject* exclude) {
    int count = m_collisionObjects.size();
    for (int i = 0; i < count; i++) {
        GameObject* obj = m_collisionObjects.elementAt(i);
        if (obj == exclude || !obj->isActive()) {
            continue;
        }
        obj->validateCollShape();
        float tMin = 0.0f;
        float tMax = 1.0f;
        if (obj->getShape()->intersects(line, &tMin, &tMax) &&
            GameCommon::boundsIntersect(0.0f, 1.0f, tMin, tMax)) {
            return obj;
        }
    }
    return NULL;
}

bool MEdgeMap::calculateCollision(const MathLine& line, float& distance, int layer) {
    bool found = false;
    distance = 1e9f;
    float t = 0.0f;
    int start = 0;
    int end = 0;
    getAttribSectionBounds(layer, line.m_origin.x, start, end);
    for (int i = start; i != end; i++) {
        if (m_shapes[i]->intersects(line, &t, NULL) && t >= 0.0f) {
            if (!found || t < distance) {
                distance = t;
                found = true;
            }
        }
    }
    return found;
}

bool MEdgeMap::intersects(const MathLine& line, int layer, float& tMin, float& tMax) {
    float rangeMin = tMin;
    float rangeMax = tMax;
    int start = 0;
    int end = 0;
    getAttribSectionBounds(layer, line, tMin, tMax, start, end);
    float hitMin = 0.0f;
    float hitMax = 0.0f;
    if (start == end) {
        return false;
    }

    bool found = false;
    for (int i = start; i != end; i++) {
        if (m_shapes[i]->intersects(line, &hitMin, &hitMax) &&
            GameCommon::boundsIntersect(hitMin, hitMax, rangeMin, rangeMax)) {
            if (found) {
                tMin = midp::Math::min(tMin, hitMin);
                tMax = midp::Math::max(tMax, hitMax);
            } else {
                tMin = hitMin;
                tMax = hitMax;
                found = true;
            }
        }
    }
    if (!found) {
        return false;
    }
    for (int i = start; i != end; i++) {
        if (m_shapes[i]->intersects(line, &hitMin, &hitMax) &&
            GameCommon::boundsIntersect(hitMin, hitMax, tMin, tMax)) {
            tMin = midp::Math::min(tMin, hitMin);
            tMax = midp::Math::max(tMax, hitMax);
        }
    }
    return found;
}

CollShape* MEdgeMap::getAttributeShapeAt(const MathLine& line, int layer) {
    CollShape* result = NULL;
    int start = 0;
    int end = 0;
    getAttribSectionBounds(layer, line.m_origin.x, start, end);
    float tMin = 0.0f;
    float tMax = 0.0f;
    float best = -1.0f;
    for (int i = start; i != end; i++) {
        CollShape* shape = m_shapes[i];
        if (!shape->intersects(line, &tMin, &tMax)) {
            continue;
        }
        if (tMin <= 0.0f && tMax >= 0.0f) {
            result = shape;
            best = 0.0f;
        } else if (tMin >= 0.0f && (best < 0.0f || tMin < best)) {
            result = shape;
            best = tMin;
        }
    }
    if (best < 0.0f) {
        return NULL;
    }
    return result;
}

void MEdgeMap::updateCollectableList() {
    int count = m_collectables.size();
    for (int i = 0; i < count; i++) {
        GameObject* obj = m_collectables.elementAt(i);
        if (obj != NULL && !obj->isActive()) {
            m_collectables.setElementAt(NULL, i);
        }
    }
}

void MEdgeMap::singlePlayerCheckpointActivated() {
    for (int i = m_staticObjects.size() - 1; i >= 0; i--) {
        m_staticObjects.elementAt(i)->checkpointActivated();
    }
    updateCollectableList();
}

void MEdgeMap::freeMap() {
    m_staticObjects.removeAllElements();
    m_dynamicObjects.removeAllElements();
    m_collectables.removeAllElements();
    m_updateObjects.removeAllElements();
    m_playerCollisionObjects.removeAllElements();
    m_objectCollisionObjects.removeAllElements();
    m_collisionObjects.removeAllElements();
    int count = m_shapes.m_length;
    for (int i = 0; i < count; i++) {
        midp::DECREF(m_shapes[i]);
        m_shapes[i] = NULL;
    }
    m_mapGroup = midp::ReferenceCountedPointer<m3g::Group>(NULL);
    m_skyDome = midp::ReferenceCountedPointer<m3g::Node>(NULL);
}

void MEdgeMap::removeCullObject(GameObjectCulledGraphic* graphic) {
    m_cullObjects.removeElement(graphic);
}

bool MEdgeMap::isInPainBox(GameObject* obj) {
    for (int i = 0; i < m_painBoxes.m_length; i++) {
        if (m_painBoxes[i]->intersects(*obj)) {
            return true;
        }
    }
    return false;
}

bool MEdgeMap::isInFallBox(GameObject* obj) {
    for (int i = 0; i < m_fallBoxes.m_length; i++) {
        if (m_fallBoxes[i]->contains(obj->m_position)) {
            return true;
        }
    }
    return false;
}

int MEdgeMap::moveObjectPhysics(MathVector& position, MathVector& velocity, float dt, CollShape* shape,
                                CollShape** hit, MathVector& normal) {
    int result = 0;
    while (dt != 0.0f && (result & HIT_SLOPES) == 0) {
        if (velocity.isZero()) {
            break;
        }
        MathVector move(velocity);
        move *= dt;
        MathVector target(position);
        target += move;
        shape->setPosition(target);

        int start = 0;
        int end = 0;
        const MathOrthoBox& bounds = shape->getBounds();
        getAttribSectionBounds(LAYER_SOLID, bounds.m_min.x, bounds.m_max.x, start, end);
        float fraction = 0.0f;
        if (!CollShape::moveShape(shape, position, move, m_shapes, start, end, fraction, hit, normal)) {
            dt = 0.0f;
            continue;
        }

        result |= hitSides(normal);
        if (normal.y > HIT_EPSILON) {
            if (normal.x > HIT_EPSILON) {
                result |= HIT_FLOOR_SLOPE_LEFT;
            } else if (normal.x < -HIT_EPSILON) {
                result |= HIT_FLOOR_SLOPE_RIGHT;
            }
        } else if (normal.y < -HIT_EPSILON) {
            if (normal.x > HIT_EPSILON) {
                result |= HIT_CEILING_SLOPE_LEFT;
            } else if (normal.x < -HIT_EPSILON) {
                result |= HIT_CEILING_SLOPE_RIGHT;
            }
        }

        MathVector bounce(normal);
        bounce *= -bounce.dot(velocity);
        MathVector slide(velocity);
        slide += bounce;
        bounce *= shape->getBounce() * (*hit)->getBounce();
        slide *= shape->getInverseResistance() * (*hit)->getInverseResistance();
        velocity.set(bounce);
        velocity += slide;
        if (!normal.isOrthogonal() && normal.y > 0.0f) {
            velocity.y += SLOPE_LIFT;
        }
        if (fraction >= 0.0f) {
            dt = (1.0f - fraction) * dt;
        }
    }
    return result;
}

int MEdgeMap::moveObject(MathVector& position, MathVector& velocity, float dt, CollShape* shape) {
    MathVector move(velocity);
    move *= dt;
    MathVector target(position);
    target += move;
    shape->setPosition(target);

    int start = 0;
    int end = 0;
    const MathOrthoBox& bounds = shape->getBounds();
    getAttribSectionBounds(LAYER_SOLID, bounds.m_min.x, bounds.m_max.x, start, end);
    float fraction = 0.0f;
    CollShape* hit = NULL;
    MathVector normal;
    int result = 0;
    if (CollShape::moveShape(shape, position, move, m_shapes, start, end, fraction, &hit, normal)) {
        result = hitSides(normal);
    }
    return result;
}

float MEdgeMap::calculateCollisionMoveOut(const MathLine& line, int layer) {
    int start = 0;
    int end = 0;
    getAttribSectionBounds(layer, line.m_origin.x, start, end);
    float result = 0.0f;
    for (int i = start; i != end; i++) {
        if (m_shapes[i]->intersects(line.m_origin)) {
            float tMax = 0.0f;
            m_shapes[i]->intersects(line, NULL, &tMax);
            result = tMax;
            break;
        }
    }
    return result;
}

bool MEdgeMap::intersects(const MathVector& point, int layer, CollShape** hit) {
    int start = 0;
    int end = 0;
    getAttribSectionBounds(layer, point.x, start, end);
    for (int i = start; i != end; i++) {
        if (m_shapes[i]->intersects(point)) {
            if (hit != NULL) {
                *hit = m_shapes[i];
            }
            return true;
        }
    }
    return false;
}

bool MEdgeMap::intersects(const MathVector& point, int layer) {
    return intersects(point, layer, NULL);
}

bool MEdgeMap::intersects(CollShape* shape, int layer) {
    int start = 0;
    int end = 0;
    const MathOrthoBox& bounds = shape->getBounds();
    getAttribSectionBounds(layer, bounds.m_min.x, bounds.m_max.x, start, end);
    for (int i = start; i != end; i++) {
        if (CollShape::testIntersection(m_shapes[i], shape)) {
            return true;
        }
    }
    return false;
}

void MEdgeMap::updateObjectCollisions() {
    int count = m_objectCollisionObjects.size();
    for (int i = 0; i < count; i++) {
        GameObject* a = m_objectCollisionObjects.elementAt(i);
        if (!a->isActive()) {
            continue;
        }
        a->validateCollShape();
        CollShape* shapeA = a->getShape();
        for (int j = 0; j < i; j++) {
            GameObject* b = m_objectCollisionObjects.elementAt(j);
            if (b->isActive() && CollShape::testIntersection(shapeA, b->getShape())) {
                a->collidedWith(b);
                b->collidedWith(a);
            }
        }
    }

    int numPlayers = m_players.size();
    int numObjects = m_playerCollisionObjects.size();
    for (int i = 0; i < numPlayers; i++) {
        GameObjectPlayer* player = m_players.elementAt(i);
        if (!player->isActive()) {
            continue;
        }
        CollShape* playerShape = player->getShape();
        for (int j = 0; j < numObjects; j++) {
            GameObject* obj = m_playerCollisionObjects.elementAt(j);
            if (!obj->isActive()) {
                continue;
            }
            obj->validateCollShape();
            if (CollShape::testIntersection(playerShape, obj->getShape())) {
                player->collidedWith(obj);
                obj->collidedWith(player);
            }
        }
    }
}

void MEdgeMap::updateChunkVisibility(int dt) {
    m_chunkManager.update((float)dt * 0.001f);
}

void MEdgeMap::removeDynamicObject(GameObject* obj) {
    int type = obj->getType();
    int group = AppEngine::getGameObjectData()->getCollidesWith(type);
    if (obj->isFlagSet(FLAG_UPDATE)) {
        m_updateObjects.removeElement(obj);
    }
    if (group == COLLIDES_WITH_OBJECTS) {
        m_objectCollisionObjects.removeElement(obj);
        m_collisionObjects.removeElement(obj);
    } else if (group == COLLIDES_WITH_PLAYERS) {
        m_playerCollisionObjects.removeElement(obj);
        m_collisionObjects.removeElement(obj);
    }
    m_dynamicObjects.removeElement(obj);
}

void MEdgeMap::updateObjects(int dt) {
    int count = m_updateObjects.size();
    for (int i = 0; i < count; i++) {
        GameObject* obj = m_updateObjects.elementAt(i);
        obj->update(dt);
        obj->updateAnimation(dt);
    }
    for (int i = m_dynamicObjects.size() - 1; i >= 0; i--) {
        GameObject* obj = m_dynamicObjects.elementAt(i);
        if (!obj->isActive()) {
            removeDynamicObject(obj);
        }
    }

    m_scrollOffset1 += (float)dt * SCROLL_SPEED_1;
    m_scrollOffset2 += (float)dt * SCROLL_SPEED_2;
    while (m_scrollOffset1 < 0.0f) {
        m_scrollOffset1 += 1.0f;
    }
    while (m_scrollOffset2 < 0.0f) {
        m_scrollOffset2 += 1.0f;
    }
    if (m_scrollTexture1 != NULL) {
        m_scrollTexture1->setTranslation(0.0f, m_scrollOffset1, 0.0f);
    }
    if (m_scrollTexture2 != NULL) {
        m_scrollTexture2->setTranslation(0.0f, m_scrollOffset2, 0.0f);
    }
}

int MEdgeMap::getUniqueGameObjectID(GameObject* obj) {
    int level = AppEngine::getLevelData()->getCurrentLevelIndex();
    return (level << 16) | obj->getID();
}

void MEdgeMap::resetCheckpoint() {
    for (int i = m_staticObjects.size() - 1; i >= 0; i--) {
        m_staticObjects.elementAt(i)->resetCheckpoint();
    }
    for (int i = m_dynamicObjects.size() - 1; i >= 0; i--) {
        removeDynamicObject(m_dynamicObjects.elementAt(i));
    }
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    if (scene->numGameScreens() != 1) {
        return;
    }
    GameObjectPlayer* player = scene->getGameScreen(0)->getPlayerObject();
    player->resetNumCollectablesCollected();
    for (int i = m_collectables.size() - 1; i >= 0; i--) {
        GameObject* obj = m_collectables.elementAt(i);
        if (obj == NULL) {
            player->incNumCollectablesCollected();
        } else {
            obj->resetCheckpoint();
        }
    }
}

void MEdgeMap::updateObjectCulling(int screen) {
    GameScreen* gameScreen = AppEngine::getCanvas()->getSceneGame()->getGameScreen(screen);
    MathFrustum* frustum = gameScreen->getCameraFrustum();
    int count = m_cullObjects.size();
    for (int i = 0; i < count; i++) {
        m_cullObjects.elementAt(i)->calcCull(screen, *frustum);
    }
    gameScreen->getPlayerObject()->getObjectNodeCull()->forceNoCull(screen);
}

void MEdgeMap::addCullObject(GameObjectCulledGraphic* graphic) {
    m_cullObjects.addElement(graphic);
}

void MEdgeMap::removeObject(GameObject* obj) {
    if (obj->isFlagSet(FLAG_COLLECTABLE)) {
        SpywareManager::getInstance()->trackBagPickedUp(getUniqueGameObjectID(obj));
    }
    obj->deactivate();
}

void MEdgeMap::addObject(GameObject* obj) {
    if (obj->getID() == -1) {
        obj->setID(m_nextObjectID);
        m_nextObjectID++;
    }
    int type = obj->getType();
    int group = AppEngine::getGameObjectData()->getCollidesWith(type);
    if (obj->isFlagSet(FLAG_PLAYER)) {
        GameObjectPlayer* player = static_cast<GameObjectPlayer*>(obj);
        AppEngine::getCanvas()->getSceneGame()->addPlayerObject(player);
        m_players.addElement(player);
    }
    if (obj->isFlagSet(FLAG_UPDATE)) {
        m_updateObjects.addElement(obj);
    }
    if (group == COLLIDES_WITH_OBJECTS) {
        m_objectCollisionObjects.addElement(obj);
        m_collisionObjects.addElement(obj);
    } else if (group == COLLIDES_WITH_PLAYERS) {
        m_playerCollisionObjects.addElement(obj);
        m_collisionObjects.addElement(obj);
    }
    if (obj->isFlagSet(FLAG_COLLECTABLE)) {
        m_collectables.addElement(obj);
    } else if (m_loading) {
        obj->markObjectStatic();
        m_staticObjects.addElement(obj);
    } else {
        m_dynamicObjects.addElement(obj);
    }
}

void MEdgeMap::resetLevel() {
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    scene->resetCheckPointsLevel();
    for (int i = m_staticObjects.size() - 1; i >= 0; i--) {
        m_staticObjects.elementAt(i)->resetLevel();
    }
    for (int i = m_dynamicObjects.size() - 1; i >= 0; i--) {
        removeDynamicObject(m_dynamicObjects.elementAt(i));
    }
    if (scene->numGameScreens() == 1) {
        GameObjectPlayer* player = scene->getGameScreen(0)->getPlayerObject();
        player->resetNumCollectablesCollected();
        for (int i = m_collectables.size() - 1; i >= 0; i--) {
            GameObject* obj = m_collectables.elementAt(i);
            if (obj == NULL) {
                player->incNumCollectablesCollected();
            } else {
                obj->resetLevel();
            }
        }
    } else {
        for (int i = m_collectables.size() - 1; i >= 0; i--) {
            m_collectables.elementAt(i)->resetLevel();
        }
    }
}

void MEdgeMap::loadMap(int resId, m3g::World* world) {
    SceneGame* scene = AppEngine::getCanvas()->getSceneGame();
    M3GAssets* assets = AppEngine::getM3GAssets();
    LevelData* levelData = AppEngine::getLevelData();
    const Level* level = levelData->getLevel(levelData->getCurrentLevelIndex());
    int gameMode = levelData->getGameMode();
    bool raceMode = levelData->isRaceMode();
    bool multiplayer = levelData->isMultiplayerMode();
    m_loading = true;

    m_mapGroup = new m3g::Group();
    M3GAssets::addNode(world, m_mapGroup);

    AppEngine::getCanvas();
#ifdef MEDECOMP_PORT
    midp::InputStream* playerMap = medecomp_player_map();
    midp::DataInputStream in(playerMap != NULL ? playerMap : ResourceManager::loadBinaryFile(resId));
#else
    midp::DataInputStream in(ResourceManager::loadBinaryFile(resId));
#endif

    int paletteIndex = in.readInt();
    m_unk004 = in.readByte();
    m_modelSet = const_cast<ModelSet*>(levelData->getModelSet(LEVEL_DATA_LOOKUP_ARRAY[in.readInt()]));
    m_minX = readFixed(in);
    m_maxX = readFixed(in);
    m_sectionWidth = readFixed(in);

    int numSections = in.readShort();
    m_numSections = numSections;
    m_sectionStart = midp::array<short>(numSections * NUM_LAYERS);
    m_sectionEnd = midp::array<short>(numSections * NUM_LAYERS);
    for (int i = 0; i != numSections * NUM_LAYERS; i++) {
        m_sectionStart[i] = in.readShort();
        m_sectionEnd[i] = in.readShort();
    }

    short numShapes = m_sectionEnd[m_sectionEnd.m_length - 1];
    m_shapes = midp::array<CollShape*>(numShapes);
    for (int i = 0; i != numShapes; i++) {
        float minX = readFixed(in);
        float minY = readFixed(in);
        float maxX = readFixed(in);
        float maxY = readFixed(in);
        int type = in.readByte();
#ifdef MEDECOMP_PORT
        int material = MATERIAL_LOOKUP_ARRAY[in.readInt() & 0xff];
#else
        int material = MATERIAL_LOOKUP_ARRAY[in.readInt()];
#endif
        CollShape* shape;
        if (type == CollShape::SHAPE_RAMP_POS_X) {
            shape = new CollOrthoRampPosX(minX, minY, COLLISION_BOX_MIN, maxX, maxY, COLLISION_BOX_MAX);
        } else if (type == CollShape::SHAPE_RAMP_NEG_X) {
            shape = new CollOrthoRampNegX(minX, minY, COLLISION_BOX_MIN, maxX, maxY, COLLISION_BOX_MAX);
        } else if (type == CollShape::SHAPE_ORTHO_HEXAHEDRON) {
            shape = new CollOrthoHexahedron(minX, minY, COLLISION_BOX_MIN, maxX, maxY, COLLISION_BOX_MAX);
        } else {
            shape = NULL;
        }
        shape->m_material = material;
        m_shapes[i] = shape;
        midp::INCREF(shape);
    }
#ifdef MEDECOMP_PORT
    snapFloorTops(m_shapes);
#endif

    MapPalette palette(RES_LOOKUP_ARRAY[paletteIndex], m_modelSet);
    m_chunkManager.load(in, palette, m_mapGroup);
#ifdef MEDECOMP_PORT
    if (medecomp_player_level_playing()) medecomp_level_supports(this, m_mapGroup, &palette);
#endif

    int numPaths = in.readShort();
    m_paths = midp::array<Path*>(numPaths);
    for (int i = 0; i < numPaths; i++) {
        m_paths[i] = new Path(in);
        midp::INCREF(m_paths[i]);
    }
    int numFallBoxes = in.readShort();
    m_fallBoxes = midp::array<FallBox*>(numFallBoxes);
    for (int i = 0; i < numFallBoxes; i++) {
        m_fallBoxes[i] = new FallBox(in);
        midp::INCREF(m_fallBoxes[i]);
    }
    int numPainBoxes = in.readShort();
    m_painBoxes = midp::array<PainBox*>(numPainBoxes);
    for (int i = 0; i < numPainBoxes; i++) {
        m_painBoxes[i] = new PainBox(in);
        midp::INCREF(m_painBoxes[i]);
    }

    for (int n = in.readUnsignedShort(); n != 0; n--) {
        int type = in.readInt();
        float x = readFixed(in);
        float y = readFixed(in);
        float z = readFixed(in);
        int pathIndex = in.readInt();
        midp::ReferenceCountedPointer<m3g::Node> graphic;
        ChunkRunnerVision* vision = NULL;
        if ((unsigned char)in.readByte() == 1) {
            graphic = m_chunkManager.getGraphicsReference(in, &vision);
        }
        GameObjectData* data = AppEngine::getGameObjectData();
        GameObject* obj = NULL;
        switch (type) {
        case MAP_OBJECT_PIGEONS:
            obj = new GameObjectEffectPigeonTakeoff(this, x, y, z);
            break;
        case MAP_OBJECT_CHOPPER:
            if (!multiplayer) {
                obj = new GameObjectChopper(this, x, y, z, pathIndex);
            }
            break;
        case MAP_OBJECT_POLICE_LIGHT:
            obj = new GameObjectPoliceLight(this, x, y, z, pathIndex);
            break;
        case MAP_OBJECT_RIVAL_BOSS:
            obj = new GameObjectRivalBoss(this, x, y, z, pathIndex);
            break;
        case MAP_OBJECT_PLANE_LIGHT:
            obj = new GameObjectPlaneLight(this, x, y, z);
            break;
        case MAP_OBJECT_RIVAL_GENERAL:
            obj = new GameObjectRivalGeneral(this, x, y, z, pathIndex);
            break;
        case MAP_OBJECT_POLICE_RIOT:
            obj = new GameObjectPoliceRiot(this, x, y, z, pathIndex);
            break;
        default: {
            int objType = MAP_OBJECT_IDS[type];
            if (data->getFlags(objType) & FLAG_COLLECTABLE) {
                midp::ReferenceCountedPointer<GameObject> bag(
                    new GameObjectCollectable(this, palette, objType, x, y, z));
                if (!raceMode) {
                    bool found = level->isBagFound(m_collectables.size());
#ifdef MEDECOMP_PORT
                    if (medecomp_player_level_playing()) {
                        found = false;
                    }
#endif
                    if (found) {
                        m_collectables.addElement(NULL);
                        m_nextObjectID++;
                    } else {
                        addObject(bag);
                    }
                }
            } else {
                obj = new GameObject(this, objType, x, y, z);
            }
            break;
        }
        }
        if (obj != NULL) {
            addObject(obj);
        }
    }

    int numStarts = in.readUnsignedByte();
    int myStart = 0;
#ifdef MEDECOMP_PORT
    if (port::net::rivalsNetwork() && !port::net::raceIsHost() && numStarts > 1) {
        myStart = 1;
    }
#endif
    for (int i = 0; i < numStarts; i++) {
        float x = readFixed(in);
        float y = readFixed(in);
        float z = readFixed(in);
        GameObjectRunner::FacingDir facing = (GameObjectRunner::FacingDir)in.readByte();
        if (i == myStart) {
            addObject(new GameObjectPlayer(this, 0, 0, x, y, z, facing));
        }
        if (scene->numGameScreens() > 1 && (i == 1 || numStarts == 1)) {
            addObject(new GameObjectPlayer(this, 1, 1, x, y, z, facing));
        }
    }

    int numFinishes = in.readUnsignedByte();
    for (int i = 0; i < numFinishes; i++) {
        float x = readFixed(in);
        float y = readFixed(in);
        float width = readFixed(in);
        float height = readFixed(in);
        if (gameMode != LevelData::GAME_MODE_MULTIPLAYER_TIMED) {
            addObject(new GameObjectFinish(this, x, y, width, height));
        }
    }

    int numCheckpoints = in.readUnsignedByte();
    for (int i = 0; i < numCheckpoints; i++) {
        float minX = readFixed(in);
        float minY = readFixed(in);
        float maxX = readFixed(in);
        float maxY = readFixed(in);
        ChunkRunnerVision* vision = NULL;
        midp::ReferenceCountedPointer<m3g::Node> graphic(m_chunkManager.getGraphicsReference(in, &vision));
        m3g::Node* node = graphic;
        bool facingRight = vision->getType() != ChunkRunnerVision::TYPE_LEFT;
        addObject(new GameObjectCheckpoint(this, m_modelSet, minX, minY, maxX, maxY, node, facingRight));
    }

    int numAmbientSounds = in.readUnsignedShort();
    for (int i = 0; i < numAmbientSounds; i++) {
        int soundId = RES_LOOKUP_ARRAY[in.readInt()];
        float x = readFixed(in);
        float y = readFixed(in);
        float z = readFixed(in);
        addObject(new GameObjectAmbientSound(this, soundId, x, y, z));
    }
    int numOneShots = in.readUnsignedShort();
    for (int i = 0; i < numOneShots; i++) {
        int index = in.readInt();
        float x = readFixed(in);
        float y = readFixed(in);
        float z = readFixed(in);
        if (!multiplayer) {
            addObject(new GameObjectSoundOneShot(this, RES_LOOKUP_ARRAY[index], x, y, z));
        }
    }
    int numMusicCues = in.readUnsignedShort();
    for (int i = 0; i < numMusicCues; i++) {
        int index = in.readInt();
        float x = readFixed(in);
        float y = readFixed(in);
        float z = readFixed(in);
        if (!multiplayer) {
            addObject(new GameObjectMusicCue(this, RES_LOOKUP_ARRAY[index], x, y, z));
        }
    }

    for (signed char n = in.readByte(); n != 0; n--) {
        float x1 = readFixed(in);
        float y1 = readFixed(in);
        float x2 = readFixed(in);
        float y2 = readFixed(in);
        float endOffset = readFixed(in);
        addObject(new GameObjectZipLine(this, palette, x1, y1, x2, y2, endOffset));
    }

    for (short n = in.readShort(); n != 0; n--) {
        int index = in.readInt();
        float minX = readFixed(in);
        float minY = readFixed(in);
        float maxX = readFixed(in);
        float maxY = readFixed(in);
        if (gameMode == LevelData::GAME_MODE_STORY) {
            addObject(new GameObjectPopupBox(this, MAP_STRING_IDS[index], minX, minY, maxX, maxY));
        }
    }
    for (short n = in.readShort(); n != 0; n--) {
        int index = in.readInt();
        float minX = readFixed(in);
        float minY = readFixed(in);
        float maxX = readFixed(in);
        float maxY = readFixed(in);
        if (gameMode == LevelData::GAME_MODE_STORY) {
            addObject(new GameObjectCutsceneTrigger(this, MAP_CUTSCENE_IDS[index], minX, minY, maxX, maxY));
        }
    }

    int numCutscenes = in.readShort();
    m_cutscenes = midp::array<midp::ReferenceCountedPointer<DataCameraAnim> >(numCutscenes);
    for (int i = 0; i < numCutscenes; i++) {
        m_cutscenes[i] = new DataCameraAnim(in);
    }
    in.close();

    m_skyDome = palette.getNode(NODE_SKY_DOME);
    if (m_skyDome != NULL) {
        M3GAssets::addNode(world, m_skyDome);
    }
    m3g::Appearance* appearance = assets->getTexturedAppearance(APPEARANCE_SCROLL_1);
    if (appearance != NULL) {
        m_scrollTexture1 = appearance->getTexture(0);
    }
    appearance = assets->getTexturedAppearance(APPEARANCE_SCROLL_2);
    if (appearance != NULL) {
        m_scrollTexture2 = appearance->getTexture(0);
    }
    m_scrollOffset1 = 0.0f;
    m_scrollOffset2 = 0.0f;
    m_loading = false;
}

MEdgeMap::~MEdgeMap() {
    int count = m_shapes.m_length;
    for (int i = 0; i < count; i++) {
        midp::DECREF(m_shapes[i]);
        m_shapes[i] = NULL;
    }
    count = m_paths.m_length;
    for (int i = 0; i < count; i++) {
        midp::DECREF(m_paths[i]);
        m_paths[i] = NULL;
    }
    count = m_fallBoxes.m_length;
    for (int i = 0; i < count; i++) {
        midp::DECREF(m_fallBoxes[i]);
        m_fallBoxes[i] = NULL;
    }
    count = m_painBoxes.m_length;
    for (int i = 0; i < count; i++) {
        midp::DECREF(m_painBoxes[i]);
        m_painBoxes[i] = NULL;
    }
    m_cullObjects.~VectorSE();
    m_collisionObjects.~VectorSE();
    m_objectCollisionObjects.~VectorSE();
    m_playerCollisionObjects.~VectorSE();
    m_updateObjects.~VectorSE();
    m_collectables.~VectorSE();
    m_players.~VectorSE();
    m_dynamicObjects.~VectorSE();
    m_staticObjects.~VectorSE();
}

MEdgeMap::MEdgeMap()
    : m_nextObjectID(0),
      m_unk004(-1),
      m_modelSet(NULL),
      m_minX(0.0f),
      m_maxX(0.0f),
      m_sectionWidth(0.0f),
      m_numSections(0),
      m_loading(false),
      m_staticObjects(),
      m_dynamicObjects(),
      m_players(),
      m_collectables(),
      m_updateObjects(),
      m_playerCollisionObjects(),
      m_objectCollisionObjects(),
      m_collisionObjects(),
      m_cullObjects(),
      m_scrollTexture1(NULL),
      m_scrollTexture2(NULL),
      m_scrollOffset1(0.0f),
      m_scrollOffset2(0.0f) {}

int MEdgeMap::getAttribSectionOffset(int layer, float x) {
    int section = midp::Math::max(0, midp::Math::min(m_numSections - 1, (int)((x - m_minX) / m_sectionWidth)));
    return m_numSections * layer + section;
}

int MEdgeMap::getModelId(int index) {
    return m_modelSet->getModelId(index);
}

int MEdgeMap::getNumPlayerCheckObjects() {
    return m_playerCollisionObjects.size();
}

GameObject* MEdgeMap::getPlayerCheckObject(int index) {
    return m_playerCollisionObjects.elementAt(index);
}

Path* MEdgeMap::getPath(int index) {
    return m_paths[index];
}

m3g::Node* MEdgeMap::getSkyDomeNode() {
    return m_skyDome;
}
