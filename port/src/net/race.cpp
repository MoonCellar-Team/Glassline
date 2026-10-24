#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "game/AppEngine.h"
#include "game/CollShape.h"
#include "game/GameCamera.h"
#include "game/GameObjectCollectable.h"
#include "game/GameObjectBullet.h"
#include "game/GameObjectCulledGraphic.h"
#include "game/GameObjectPlayer.h"
#include "game/GameObjectPolice.h"
#include "game/GameObjectRunner.h"
#include "game/GameScreen.h"
#include "game/LevelData.h"
#include "game/M3GAssets.h"
#include "game/MathLine.h"
#include "game/MEdgeMap.h"
#include "game/SceneGame.h"
#include "game/SoundEventPoolManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "game/UI/Window.h"
#include "game/UI/WindowStore.h"
#include "m3g/Node.h"
#include "../ea/levels.h"
#include "../platform/levels.h"
#include "../platform/overlay.h"
#include "../platform/settings.h"
#include "../platform/skins.h"
#include "port/files.h"
#include "port/net.h"
#include "runtime/midp/Graphics.h"
#include "text.h"

namespace port {
namespace net {

namespace {

const uint8_t kVersion = kNetVersion;
enum : uint8_t { kHello = 1, kState = 2, kEvent = 3, kReady = 4, kGo = 5,
                 kBag = 6, kBagDeny = 7, kRivalHit = 8, kGift = 9, kTimer = 10, kEnd = 11,
                 kSkin = 12, kSkinPart = 13,
                 kQuit = 14,
                 kEnemy = 15,
                 kLevel = 16, kLevelPart = 17, kLevelNeed = 18,
                 kChoice = 19,
                 kLevelState = 20 };
enum : uint8_t { kMapServer = 1, kMapFile = 2, kMapReady = 3, kMapFailed = 4 };
enum : uint8_t { kCheckpoint = 1, kFinish = 2, kHit = 3 };
const int kSendEveryMs = 50;
const int kDelayMs = 100;
const int kVisualKnocked = 10, kVisualLying = 33, kVisualGettingUp = 34;
const int kPredictSlackMs = 400;
const int kSettleMs = 250;
const int kCatchupMs = 700;
const int kTypeGhost = 2;
const int kSceneGame = 3;
const int kHoldMs = 45000;
const int kWinAfterMs = 1500;
const int kTopMessage = -3;
const int kNoText = 0x800;
const int kWaitingY = 150;

const int TEXT_WAITING = 0x98c;
const int TEXT_PEER_GETTING_MAP = 0x9cb;
const int TEXT_PEER_MAP_FAILED = 0x9cc;
const int kTransferQuietMs = 30000;
const int kMapFailShowMs = 4000;
const int TEXT_FINISHED = 0x98d;
const int TEXT_YOU_WIN = 0x98e;
const int TEXT_PEER_WINS = 0x98f;
const int TEXT_DRAW = 0x990;
const int TEXT_RUNNING = 0x991;
const int TEXT_LEFT = 0x992;
const int TEXT_HOST_DECIDES = 0x99d;
const int TEXT_RESTARTED = 0x9be;
const int kQuitAfterMs = 2500;
const int TEXT_MULTIPLAYER = 0x930;
const int TEXT_ROOM_OPEN = 0x9d3;
const int TEXT_JOINED = 0x9d0;
const int TEXT_PEER_QUIT = 0x9d1;
const int TEXT_LOST = 0x9d2;
const int TEXT_PEER_READY = 0x9d4;
const int TEXT_PEER_IN_MENU = 0x9d5;
const int TEXT_FOLLOWING = 0x9d6;
const int TEXT_ALONE = 0x9d7;
const char* const kNormalClose = "до встречи";
const int kMenuNoticeMs = 1500;

const int FONT_HEADING = 0x1c;
const int FONT_NOTE = 0x1d;
const int FONT_ROW_LABEL = 0x22;
const int FONT_ROW_TIME = 0x21;
const int ANCHOR_CENTER_BASELINE = 0x42;
const int ANCHOR_LEFT_BASELINE = 0x41;
const int ANCHOR_RIGHT_BASELINE = 0x44;

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string secondsText(int ms) {
    char buf[32];
    snprintf(buf, sizeof buf, "%s%d.%03d", ms < 0 ? "-" : "", (ms < 0 ? -ms : ms) / 1000, (ms < 0 ? -ms : ms) % 1000);
    return buf;
}

int verbose() {
    static const int level = getenv("MEDECOMP_NET_LOG") ? atoi(getenv("MEDECOMP_NET_LOG")) : 0;
    return level;
}

struct Writer {
    std::vector<uint8_t> b;
    void u8(uint8_t v) { b.push_back(v); }
    void str(const std::string& s) {
        size_t n = std::min<size_t>(s.size(), 0xffff);
        u16(uint16_t(n));
        b.insert(b.end(), s.begin(), s.begin() + long(n));
    }
    void u16(uint16_t v) { u8(uint8_t(v)), u8(uint8_t(v >> 8)); }
    void u32(uint32_t v) { u16(uint16_t(v)), u16(uint16_t(v >> 16)); }
    void f32(float v) {
        uint32_t u;
        memcpy(&u, &v, 4);
        u32(u);
    }
};

struct Reader {
    const std::vector<uint8_t>& b;
    size_t at;
    bool ok = true;
    uint8_t u8() {
        if (at >= b.size()) {
            ok = false;
            return 0;
        }
        return b[at++];
    }
    uint16_t u16() { return uint16_t(u8() | (u8() << 8)); }
    uint32_t u32() { return uint32_t(u16()) | (uint32_t(u16()) << 16); }
    float f32() {
        uint32_t u = u32();
        float v;
        memcpy(&v, &u, 4);
        return v;
    }
    std::string str() {
        size_t n = u16();
        std::string s;
        for (size_t i = 0; i < n && ok; i++) s += char(u8());
        return s;
    }
};

struct Frame {
    long long time;
    MathVector position;
    int visual;
    float blend;
    uint8_t flags;
};

std::deque<Frame> s_frames;
std::string peerName() { return sessionInfo().peerName; }
long long s_offset;
bool s_haveOffset;
int s_remoteLevel = -1;

Peer s_peer;

const size_t kSkinFileMax = 8u << 20;
const size_t kSkinChunk = 200u << 10;

struct PeerFile {
    uint32_t size = 0, hash = 0;
    std::vector<uint8_t> data;
    size_t received = 0;
    std::string path;
};

bool s_skinSent = false, s_peerSkinKnown = false;
std::string s_peerBase;
PeerFile s_peerFiles[2];
int s_peerArrived;
bool s_closeOnLeave;

uint32_t fnv(const uint8_t* p, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

std::string peerSkinDir() { return port::files::dataDir() + "/cache/peer-skins"; }

bool readAll(const std::string& path, std::vector<uint8_t>* out) {
    if (path.empty()) return false;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out->clear();
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0 && out->size() <= kSkinFileMax) out->insert(out->end(), buf, buf + n);
    fclose(f);
    return out->size() <= kSkinFileMax;
}

port::platform::skins::Look myLook() {
    return port::platform::skins::lookOf(port::platform::settings::get("skin", ""));
}

port::platform::skins::Look peerLook() {
    port::platform::skins::Look look = port::platform::skins::lookOf(s_peerSkinKnown ? s_peerBase : std::string());
    if (!s_peerFiles[0].path.empty()) look.texture = s_peerFiles[0].path;
    if (!s_peerFiles[1].path.empty()) look.model3d = s_peerFiles[1].path;
    return look;
}

void sendSkin() {
    if (!s_peer || s_skinSent) return;
    s_skinSent = true;
    std::string key = port::platform::settings::get("skin", "");
    port::platform::skins::Look look = myLook();
    std::string base = port::platform::skins::baseKeyOf(key);
    std::vector<uint8_t> files[2];
    if (!readAll(look.texture, &files[0])) files[0].clear();
    if (!readAll(look.model3d, &files[1])) files[1].clear();
    Writer w;
    w.u8(kSkin);
    w.u8(kVersion);
    w.u16(uint16_t(base.size()));
    for (char c : base) w.u8(uint8_t(c));
    for (const std::vector<uint8_t>& f : files) {
        w.u32(uint32_t(f.size()));
        w.u32(f.empty() ? 0 : fnv(f.data(), f.size()));
    }
    send(s_peer, w.b.data(), w.b.size(), true);
    for (int kind = 0; kind < 2; kind++)
        for (size_t at = 0; at < files[kind].size(); at += kSkinChunk) {
            size_t n = std::min(kSkinChunk, files[kind].size() - at);
            Writer part;
            part.u8(kSkinPart);
            part.u8(kVersion);
            part.u8(uint8_t(kind + 1));
            part.u32(uint32_t(at));
            part.b.insert(part.b.end(), files[kind].begin() + long(at), files[kind].begin() + long(at + n));
            if (!send(s_peer, part.b.data(), part.b.size(), true)) {
                log("свой вид: %s не ушла (кусок с %zu Б)", kind == 0 ? "текстура" : "модель", at);
                break;
            }
        }
    log("свой вид — сопернику: «%s», текстура %zu Б, модель %zu Б", base.c_str(), files[0].size(), files[1].size());
}

void receivedSkin(Reader& in) {
    size_t len = in.u16();
    std::string base;
    for (size_t i = 0; i < len && in.ok; i++) base += char(in.u8());
    uint32_t sizes[2], hashes[2];
    for (int k = 0; k < 2; k++) sizes[k] = in.u32(), hashes[k] = in.u32();
    if (!in.ok) return;
    s_peerBase = base;
    s_peerSkinKnown = true;
    const char* ext[2] = {".png", ".glb"};
    for (int k = 0; k < 2; k++) {
        PeerFile& f = s_peerFiles[k];
        f = PeerFile();
        if (!sizes[k] || sizes[k] > kSkinFileMax) continue;
        f.size = sizes[k];
        f.hash = hashes[k];
        std::string path = peerSkinDir() + "/" + std::to_string(f.hash) + ext[k];
        std::vector<uint8_t> cached;
        if (readAll(path, &cached) && cached.size() == f.size && fnv(cached.data(), cached.size()) == f.hash) {
            f.path = path;
            continue;
        }
        f.data.assign(f.size, 0);
    }
    log("вид соперника: «%s», текстура %u Б, модель %u Б", base.c_str(), sizes[0], sizes[1]);
}

void receivedSkinPart(Reader& in, const std::vector<uint8_t>& b) {
    int kind = int(in.u8()) - 1;
    size_t at = in.u32();
    if (!in.ok || kind < 0 || kind > 1) return;
    PeerFile& f = s_peerFiles[kind];
    size_t n = b.size() - in.at;
    if (!f.path.empty() || f.data.empty() || at + n > f.data.size()) return;
    std::copy(b.begin() + long(in.at), b.end(), f.data.begin() + long(at));
    f.received += n;
    if (f.received < f.size) return;
    if (fnv(f.data.data(), f.data.size()) != f.hash) {
        log("вид соперника: файл не сошёлся");
        f = PeerFile();
        return;
    }
    std::string dir = peerSkinDir();
    mkdir((port::files::dataDir() + "/cache").c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    std::string path = dir + "/" + std::to_string(f.hash) + (kind == 0 ? ".png" : ".glb");
    if (FILE* out = fopen(path.c_str(), "wb")) {
        fwrite(f.data.data(), 1, f.data.size(), out);
        fclose(out);
        f.path = path;
        f.data.clear();
        s_peerArrived |= kind + 1;
        log("вид соперника: %s получена", kind == 0 ? "текстура" : "модель");
    }
}

class RemoteRunner : public GameObjectRunner {
public:
    explicit RemoteRunner(MEdgeMap* map) : GameObjectRunner(map, kTypeGhost) {
        port::platform::skins::Look look = peerLook();
        setVisualAssets(look.model < 0 ? map->getModelId(1) : look.model, 0);
        port::platform::skins::dress(m_objectNode, look);
        if (shirtForGuest(look, false)) port::platform::skins::recolorShirt(m_objectNode, look);
        M3GAssets::applyAlphaFactor(m_objectNode, 0.0f);
        m_shape = new CollOctahedron(-0.5f, 0.0f, -0.5f, 0.5f, 1.8f, 0.5f);
        m_objectNodeCull = new GameObjectCulledGraphic(m_objectNode, getGlobalBounds());
        m_map->addCullObject(m_objectNodeCull);
    }

    void redress(int arrived) {
        port::platform::skins::Look look = peerLook();
        look.textureModel = -1;
        if (!(arrived & 2)) look.model3d.clear();
        port::platform::skins::dress(m_objectNode, look);
    }

    void checkpointActivated() override {}
    void resetCheckpoint() override {}
    void resetLevel() override {}

    void predictKnockdown() {
        m_predictPos = m_position;
        m_predictAt = nowMs();
        m_predictUntil = m_predictAt + (s_peer ? ping(s_peer) : 0) + kDelayMs + kPredictSlackMs;
        int visual = hashVisual(VISUAL_ORIGIN, kVisualKnocked, getFacingDir());
        m_shownVisual = visual;
        setVisual(visual);
    }

    void catchUp() { m_catchupAt = nowMs(); }

    void update(int dt) override {
        GameObjectRunner::update(dt);
        if (s_frames.empty() || !s_haveOffset) {
            setShown(false);
            return;
        }
        long long now = nowMs();
        long long delay = kDelayMs;
        if (m_catchupAt) {
            long long since = now - m_catchupAt;
            if (since >= kCatchupMs)
                m_catchupAt = 0;
            else
                delay = kDelayMs * since / kCatchupMs;
        }
        long long at = now - s_offset - delay;
        size_t i = 0;
        while (i + 1 < s_frames.size() && s_frames[i + 1].time <= at) i++;
        const Frame& a = s_frames[i];
        MathVector position = a.position;
        float blend = a.blend;
        if (i + 1 < s_frames.size() && at > a.time) {
            const Frame& b = s_frames[i + 1];
            float t = float(at - a.time) / float(b.time - a.time);
            if (t > 1.0f) t = 1.0f;
            position.setAsLinearInterpolation(a.position, b.position, t);
            blend = a.blend + (b.blend - a.blend) * t;
        }
        bool frozen = false;
        if (m_predictUntil) {
            if (knocked(a.visual)) {
                log("падение соперника подтвердилось его кадрами через %lld мс", now - m_predictAt);
                m_predictUntil = 0;
                m_settleFrom = m_predictPos;
                m_settleAt = now;
                if ((a.visual >> 1) == (m_shownVisual >> 1)) {
                    m_shownVisual = a.visual;
                    setFacingDir(FacingDir(((a.visual & 1) << 1) - 1));
                }
            } else if (now < m_predictUntil) {
                frozen = true;
            } else {
                m_predictUntil = 0;
                log("удар у соперника не прошёл — его кадры без падения");
            }
        }
        if (frozen) {
            position = m_predictPos;
        } else {
            if (a.visual != m_shownVisual) {
                m_shownVisual = a.visual;
                setVisual(a.visual);
            }
            if (getVisualType() == VISUAL_BLEND3) set3WayAnimationBlendWeights(blend);
            if (m_settleAt) {
                long long since = now - m_settleAt;
                if (since >= kSettleMs)
                    m_settleAt = 0;
                else
                    position.setAsLinearInterpolation(m_settleFrom, position, float(since) / float(kSettleMs));
            }
        }
        bool blink = (a.flags & 2) && !frozen && !knocked(m_shownVisual) && ((now >> 8) & 1);
        if (blink != m_blinkHidden) {
            m_blinkHidden = blink;
            m_objectNodeCull->setVisible(!blink);
        }
        m_position = position;
        m_objectNode->setTranslation(m_position.x, m_position.y, m_position.z);
        while (i > 1) {
            s_frames.pop_front();
            i--;
        }
        setShown(nowMs() - s_offset - s_frames.back().time < 3000);
    }

    bool shown() const { return m_shown; }

    void setShown(bool shown) {
        if (shown == m_shown) return;
        m_shown = shown;
        M3GAssets::applyAlphaFactor(m_objectNode, shown ? 1.0f : 0.0f);
    }

private:
    static bool knocked(int visual) {
        int type = (visual >> 1) & 3, index = visual >> 3;
        return (type == VISUAL_ORIGIN && index == kVisualKnocked) ||
               (type == VISUAL_SOLO && (index == kVisualLying || index == kVisualGettingUp));
    }

    int m_shownVisual = -2;
    bool m_shown = false;
    long long m_predictAt = 0;
    long long m_predictUntil = 0;
    MathVector m_predictPos;
    long long m_settleAt = 0;
    MathVector m_settleFrom;
    long long m_catchupAt = 0;
    bool m_blinkHidden = false;
};

bool s_started;
SceneGame* s_scene;
RemoteRunner* s_remote;
long long s_lastSent;
int s_lastVisual = -2;
MathVector s_lastCheckpoint;
bool s_finished;

int s_remoteReadyLevel = -1;
bool s_remoteRacing;
int s_joinLevel = -1;
bool s_holding;
long long s_holdSince, s_goAt;
bool s_together;
int s_myFinish = -1, s_remoteFinish = -1;
long long s_myFinishAt;
bool s_remoteLeft;
long long s_quitAt;
int s_followLevel = -1;
bool s_followNewMap;
bool s_followRestart;
bool s_chooseNext;
long long s_peerMenuAt;
std::atomic<bool> s_noPause(false);

const int kTimerEveryMs = 250;
const int kHitCooldownMs = 2500;
const float kHitReachX = 1.0f, kHitReachY = 1.6f;
int s_remoteMode = LevelData::GAME_MODE_SPEEDRUN;
int s_remoteTimeLimit;
bool s_remoteCombat;
int s_remoteBags;
bool s_remoteAttacking, s_remoteImmune;
std::set<int> s_myBags;
int s_hostTime = -1;
long long s_hostTimeAt;
bool s_endReceived;
long long s_hitCooldownUntil, s_lastTimerSent;

bool rivalsMode() { return AppEngine::getLevelData()->getGameMode() == LevelData::GAME_MODE_MULTIPLAYER_TIMED; }
bool combatChosen() { return AppEngine::getLevelData()->m_multiplayerCombatAllowed; }

struct EnemyChange {
    int brain, anim;
    MathVector position, velocity;
    float facing;
};
std::map<int, EnemyChange> s_enemiesOut;

bool enemyOut(int brain) { return brain == GameObjectPolice::BRAIN_DISARMED || brain == GameObjectPolice::BRAIN_FALLEN; }

GameObjectPolice* findEnemy(int id) {
    if (!s_scene) return nullptr;
    util::VectorSE<GameObject>& objects = s_scene->getMap().m_staticObjects;
    for (int i = 0; i < objects.size(); i++) {
        GameObject* obj = objects.elementAt(i);
        int type = obj->getType();
        if (obj->getID() == id &&
            (type == GameObjectPolice::TYPE_POLICE_LIGHT || type == GameObjectPolice::TYPE_POLICE_RIOT))
            return static_cast<GameObjectPolice*>(obj);
    }
    return nullptr;
}

void applyEnemy(GameObjectPolice* cop, const EnemyChange& c) {
    int brain = cop->m_brainState;
    if (brain == c.brain || brain == GameObjectPolice::BRAIN_FALLEN) return;
    if (!enemyOut(c.brain) && !cop->isActive()) return;
    if (cop->m_state == GameObjectNPC::NPC_STATE_START) cop->stateTransition(GameObjectPolice::STATE_STAND);
    cop->m_position = c.position;
    cop->m_velocity = c.velocity;
    cop->m_knockdownPos = c.position;
    cop->m_facing = cop->m_targetFacing = c.facing;
    cop->m_facePlayer = false;
    cop->m_stateTime = c.brain == GameObjectPolice::BRAIN_MELEED_FROM_PRONE ? 650 : 0;
    if (c.brain != GameObjectPolice::BRAIN_FALLEN) {
        cop->animStateTransition(GameObjectPolice::AnimState(c.anim));
        if (c.brain != GameObjectPolice::BRAIN_DISARMED) cop->playRandomGruntSound();
    } else if (c.anim == GameObjectPolice::ANIM_LAND) {
        cop->animStateTransition(GameObjectPolice::ANIM_LAND);
        cop->stateTransition(GameObjectNPC::NPC_STATE_DISABLED);
    } else {
        bool forward = c.anim == GameObjectPolice::ANIM_KNOCKED_FORWARD ||
                       c.anim == GameObjectPolice::ANIM_KNOCKED_FORWARD_HIGH;
        cop->animStateTransition(forward ? GameObjectPolice::ANIM_FALL_FORWARD : GameObjectPolice::ANIM_FALL_BACKWARD);
    }
    cop->m_brainState = GameObjectPolice::BrainState(c.brain);
}

GameObjectPlayer* s_aimPlayer;
MathVector s_aimSaved;

bool enemyNotices(GameObjectPolice* cop, const MathVector& at) {
    int brain = cop->m_brainState;
    if (brain < GameObjectPolice::BRAIN_ALERT || brain > GameObjectPolice::BRAIN_MELEE) {
        bool ahead = at.x < cop->m_position.x ? cop->m_facing < midp::Math::PI : cop->m_facing > midp::Math::PI;
        int range = ahead ? cop->m_tuning.sightRangeAhead : cop->m_tuning.sightRangeBehind;
        MathVector to(at.x - cop->m_position.x, at.y - cop->m_position.y, at.z - cop->m_position.z);
        if (to.getLengthSq() >= float(range * range)) return false;
    }
    MathLine line(cop->m_position, at);
    line.m_direction -= line.m_origin;
    line.m_origin.y += 1.2f;
    float tMin = 0.0f;
    float tMax = 1.0f;
    return !s_scene->getMap().intersects(line, 0, tMin, tMax);
}

class RemoteBullet : public GameObjectBullet {
public:
    using GameObjectBullet::GameObjectBullet;
    void collidedWith(GameObject*) override {}
    void update(int dt) override {
        if (s_remote && fabsf(m_position.x - s_remote->m_position.x) < 0.5f && m_position.y > s_remote->m_position.y &&
            m_position.y < s_remote->m_position.y + 1.8f) {
            m_map->removeObject(this);
            return;
        }
        GameObjectBullet::update(dt);
    }
};

const char* enemyText(int brain) {
    switch (brain) {
    case GameObjectPolice::BRAIN_KNOCKED_DOWN: return "сбит";
    case GameObjectPolice::BRAIN_MELEED_FROM_PRONE: return "сбит с земли";
    case GameObjectPolice::BRAIN_DISARMED: return "обезоружен";
    default: return "упал";
    }
}

void startRun(bool together) {
    s_enemiesOut.clear();
    s_myBags.clear();
    s_hostTime = -1;
    s_holding = false;
    s_together = together;
    s_remoteRacing = together;
    s_remoteReadyLevel = -1;
    s_myFinish = -1;
    s_remoteFinish = -1;
    s_myFinishAt = 0;
    s_remoteLeft = false;
}

void topMessage(const std::string& line) {
    AppEngine* app = AppEngine::getCanvas();
    if (!app || app->m_sceneId != kSceneGame || !s_scene) return;
    TextManager* text = app->getTextManager();
    text->m_dynamicStrings[kTopMessage - TextManager::FIRST_DYNAMIC_STRING] = fromUtf8(line);
    s_scene->startTopMessage(kTopMessage, kNoText);
}

int currentLevel() { return AppEngine::getLevelData()->getCurrentLevelIndex(); }

void notice(const std::string& text) {
    log("уведомление: %s", text.c_str());
    port::platform::notify(gameText(TEXT_MULTIPLAYER), text);
}

void notice(int textId) {
    std::string name = peerName();
    notice(replaced(gameText(textId), "{name}", name.empty() ? "?" : name));
}

const size_t kLevelChunk = 200u << 10;

struct PeerLevel {
    bool expected = false;
    bool known = false;
    bool ready = false;
    port::platform::SharedLevel info;
    size_t size = 0, got = 0;
    std::string localId;
    int stage = 0;
    int mismatches = 0;
};
PeerLevel s_peerLevel;
struct GuestMap {
    int stage = 0;
    size_t got = 0, total = 0;
    long long at = 0;
} s_guestMap;
struct PartQueue {
    std::vector<unsigned char> map;
    size_t at = 0;
    long long next = 0;
} s_partQueue;

struct Browsing : HostBrowsing {
    bool operator==(const Browsing& o) const {
        return race == o.race && level == o.level && timeLimit == o.timeLimit && combat == o.combat &&
               playerLevel == o.playerLevel;
    }
};
Browsing s_browseSent, s_browseGot;
bool s_browseSentAny;
std::atomic<int> s_levelFetchState(0);
std::mutex s_levelFetchMu;
std::string s_levelFetchId;
uint64_t s_levelFetchHash;

bool sendLevelPart(const std::vector<unsigned char>& map, size_t at, size_t n) {
    Writer part;
    part.u8(kLevelPart);
    part.u8(kVersion);
    part.u32(uint32_t(at));
    part.b.insert(part.b.end(), map.begin() + long(at), map.begin() + long(at + n));
    if (getenv("MEDECOMP_NET_TEST_MAP_CORRUPT")) part.b.back() ^= 0x5a;
    if (send(s_peer, part.b.data(), part.b.size(), true)) return true;
    log("уровень игрока: кусок с %zu Б не ушёл", at);
    return false;
}

void sendLevelParts(const port::platform::SharedLevel& l) {
    if (getenv("MEDECOMP_NET_TEST_MAP_PACE")) {
        s_partQueue = PartQueue();
        s_partQueue.map = l.map;
        return;
    }
    for (size_t at = 0; at < l.map.size(); at += kLevelChunk)
        if (!sendLevelPart(l.map, at, std::min(kLevelChunk, l.map.size() - at))) return;
    log("уровень игрока — сопернику файлом, %zu Б", l.map.size());
}

void levelPartsFrame() {
    const char* pace = getenv("MEDECOMP_NET_TEST_MAP_PACE");
    if (!pace || !s_peer || s_partQueue.at >= s_partQueue.map.size() || nowMs() < s_partQueue.next) return;
    size_t n = std::min(size_t(1024), s_partQueue.map.size() - s_partQueue.at);
    if (!sendLevelPart(s_partQueue.map, s_partQueue.at, n)) return;
    s_partQueue.at += n;
    s_partQueue.next = nowMs() + atoi(pace);
    if (s_partQueue.at >= s_partQueue.map.size()) log("уровень игрока — сопернику файлом, %zu Б", s_partQueue.map.size());
}

void sendSharedLevel(const port::platform::SharedLevel& l) {
    Writer w;
    w.u8(kLevel);
    w.u8(kVersion);
    w.u32(uint32_t(l.hash));
    w.u32(uint32_t(l.hash >> 32));
    w.u32(uint32_t(l.map.size()));
    w.u8(l.bags ? 1 : 0);
    w.u32(uint32_t(l.timeLimit));
    w.u32(uint32_t(l.serverVersion));
    w.str(l.serverId);
    w.str(l.name);
    w.str(l.author);
    w.str(l.palette);
    send(s_peer, w.b.data(), w.b.size(), true);
    s_guestMap = GuestMap();
    log("уровень игрока «%s» — сопернику%s", l.name.c_str(), l.serverId.empty() ? "" : " (выложен — скачает сам)");
    if (l.serverId.empty()) sendLevelParts(l);
}

void sendHello() {
    LevelData* levels = AppEngine::getLevelData();
    port::platform::SharedLevel shared;
    bool level = sessionInfo().host && s_scene && port::platform::currentSharedLevel(&shared);
    Writer w;
    w.u8(kHello);
    w.u8(kVersion);
    w.u32(uint32_t(s_scene ? currentLevel() : -1));
    w.u8(uint8_t(levels->getGameMode()));
    w.u32(uint32_t(levels->getLevelTimeLimitMillis()));
    w.u8(combatChosen() ? 1 : 0);
    w.u8(level ? 1 : 0);
    AppEngine* app = AppEngine::getCanvas();
    w.u8(!s_scene && app && app->m_sceneId == kSceneGame ? 1 : 0);
    send(s_peer, w.b.data(), w.b.size(), true);
    if (level) sendSharedLevel(shared);
}

void askLevelFile() {
    Writer w;
    w.u8(kLevelNeed);
    w.u8(kVersion);
    send(s_peer, w.b.data(), w.b.size(), true);
}

void sendLevelState(int stage) {
    s_peerLevel.stage = stage;
    if (!s_peer) return;
    Writer w;
    w.u8(kLevelState);
    w.u8(kVersion);
    w.u32(uint32_t(s_peerLevel.info.hash));
    w.u8(uint8_t(stage));
    w.u32(uint32_t(s_peerLevel.got));
    w.u32(uint32_t(s_peerLevel.size));
    send(s_peer, w.b.data(), w.b.size(), true);
}

void peerLevelReady(const std::string& localId, const char* how) {
    s_peerLevel.ready = true;
    s_peerLevel.localId = localId;
    log("уровень игрока «%s» у меня: %s (%s)", s_peerLevel.info.name.c_str(), localId.c_str(), how);
    sendLevelState(kMapReady);
}

void peerLevelFailed(const char* why) {
    log("уровень игрока «%s»: %s — не получу", s_peerLevel.info.name.c_str(), why);
    sendLevelState(kMapFailed);
}

void receivedLevel(Reader& in) {
    port::platform::SharedLevel l;
    uint32_t lo = in.u32(), hi = in.u32();
    l.hash = uint64_t(lo) | (uint64_t(hi) << 32);
    size_t size = in.u32();
    l.bags = in.u8() != 0;
    l.timeLimit = int32_t(in.u32());
    l.serverVersion = int32_t(in.u32());
    l.serverId = in.str();
    l.name = in.str();
    l.author = in.str();
    l.palette = in.str();
    if (!in.ok || size == 0 || size > (4u << 20)) return;
    s_peerLevel = PeerLevel();
    s_peerLevel.expected = s_peerLevel.known = true;
    s_peerLevel.info = l;
    s_peerLevel.size = size;
    std::string have = port::platform::findLevelByMap(l.hash, size);
    if (!have.empty()) return peerLevelReady(have, "уже был");
    if (l.serverId.empty()) {
        s_peerLevel.info.map.assign(size, 0);
        log("уровень игрока «%s» — жду файл, %zu Б", l.name.c_str(), size);
        sendLevelState(kMapFile);
        return;
    }
    sendLevelState(kMapServer);
    if (s_levelFetchState == 1) return;
    port::online::NetLevel net;
    net.id = l.serverId;
    net.name = l.name;
    net.author = l.author;
    net.base = l.palette;
    net.mode = l.bags ? "bags" : "race";
    net.timeLimit = l.timeLimit;
    net.version = l.serverVersion;
    s_levelFetchState = 1;
    s_levelFetchHash = l.hash;
    log("уровень игрока «%s» — скачиваю с сервера (%s, версия %d)", l.name.c_str(), l.serverId.c_str(), l.serverVersion);
    std::thread([net, hash = l.hash, size] {
        std::string id, error;
        bool ok = port::online::downloadLevel(net, &id, &error);
        std::vector<unsigned char> map;
        ok = ok && port::files::read(port::files::dataDir() + "/levels/" + id + "/level.bin", &map) &&
             map.size() == size && port::platform::levelHash(map) == hash;
        std::lock_guard<std::mutex> lock(s_levelFetchMu);
        s_levelFetchId = ok ? id : std::string();
        s_levelFetchState = ok ? 2 : 3;
    }).detach();
}

void receivedLevelPart(Reader& in, const std::vector<uint8_t>& data) {
    size_t at = in.u32();
    if (!in.ok || !s_peerLevel.known || s_peerLevel.ready || s_peerLevel.stage == kMapFailed ||
        s_peerLevel.info.map.size() != s_peerLevel.size)
        return;
    size_t n = data.size() - in.at;
    if (at > s_peerLevel.size || n > s_peerLevel.size - at) return;
    std::copy(data.begin() + long(in.at), data.end(), s_peerLevel.info.map.begin() + long(at));
    s_peerLevel.got += n;
    if (s_peerLevel.got < s_peerLevel.size) return sendLevelState(kMapFile);
    if (port::platform::levelHash(s_peerLevel.info.map) != s_peerLevel.info.hash) {
        if (++s_peerLevel.mismatches >= 2) return peerLevelFailed("файл дважды пришёл с другим хешем");
        log("уровень игрока: файл пришёл, а хеш не тот — прошу заново");
        s_peerLevel.got = 0;
        sendLevelState(kMapFile);
        askLevelFile();
        return;
    }
    std::string error, id = port::platform::savePeerLevel(s_peerLevel.info, &error);
    if (id.empty()) return peerLevelFailed(error.c_str());
    s_peerLevel.info.map.clear();
    peerLevelReady(id, "файлом от хоста");
}

void levelFetchFrame() {
    int state = s_levelFetchState;
    if (state != 2 && state != 3) return;
    std::string id;
    {
        std::lock_guard<std::mutex> lock(s_levelFetchMu);
        id = s_levelFetchId;
    }
    s_levelFetchState = 0;
    if (!s_peerLevel.known || s_peerLevel.ready || s_levelFetchHash != s_peerLevel.info.hash) return;
    if (state == 2) return peerLevelReady(id, "с сервера");
    log("уровень игрока с сервера не скачался или другой версии — прошу файл у хоста");
    s_peerLevel.info.map.assign(s_peerLevel.size, 0);
    s_peerLevel.got = 0;
    sendLevelState(kMapFile);
    askLevelFile();
}

void sendPlain(uint8_t type, bool reliable = true) {
    if (!s_peer) return;
    Writer w;
    w.u8(type);
    w.u8(kVersion);
    send(s_peer, w.b.data(), w.b.size(), reliable);
}

void sendBag(uint8_t type, int index) {
    if (!s_peer) return;
    Writer w;
    w.u8(type);
    w.u8(kVersion);
    w.u16(uint16_t(index));
    send(s_peer, w.b.data(), w.b.size(), true);
}

GameScreen* myScreen() { return s_scene ? s_scene->getGameScreen(0) : nullptr; }

void remoteTookBag(int index) {
    if (!s_scene) return;
    MEdgeMap& map = s_scene->getMap();
    util::VectorSE<GameObject>& bags = map.getCollectableList();
    GameObject* bag = index >= 0 && index < bags.size() ? bags.elementAt(index) : nullptr;
    if (!bag) return;
    if (bag->isActive()) {
        map.removeObject(bag);
        log("соперник взял сумку %d", index);
    } else if (s_myBags.count(index) && sessionInfo().host) {
        sendBag(kBagDeny, index);
        log("сумку %d взяли оба — она хоста", index);
    }
}

void hitSound() {
    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = app ? app->getSceneGame() : nullptr;
    if (scene) app->getSoundManager().playEvent(scene->getSoundPoolManager().getRandomSoundEventID(5, 1, -1), 1.0f);
}

bool remoteHitMe() {
    GameScreen* screen = myScreen();
    GameObjectPlayer* player = screen ? screen->getPlayerObject() : nullptr;
    if (!player || !combatChosen() || player->isImmuneToOtherPlayer()) return false;
    player->meleeByPlayer();
    if (rivalsMode() && player->getNumCollectablesCollected() > 0) {
        screen->decNumCollectablesCollected();
        sendPlain(kGift);
    }
    return true;
}

void sendEvent(uint8_t kind, int raceTime, const MathVector& at) {
    if (!s_peer) return;
    Writer w;
    w.u8(kEvent);
    w.u8(kVersion);
    w.u8(kind);
    w.u32(uint32_t(raceTime));
    w.f32(at.x), w.f32(at.y), w.f32(at.z);
    send(s_peer, w.b.data(), w.b.size(), true);
}

void receive(const Received& r) {
    Reader in{r.data, 0};
    uint8_t type = in.u8();
    if (in.u8() != kVersion || !in.ok) return;
    if (type == kHello) {
        int wasLevel = s_remoteLevel;
        s_remoteLevel = int32_t(in.u32());
        s_remoteMode = in.u8();
        s_remoteTimeLimit = int32_t(in.u32());
        s_remoteCombat = in.u8() != 0;
        bool hostLevel = in.u8() != 0;
        bool loading = in.u8() != 0;
        s_remoteReadyLevel = -1;
        s_remoteRacing = false;
        s_joinLevel = s_remoteLevel;
        if (!sessionInfo().host) {
            s_peerLevel = PeerLevel();
            s_peerLevel.expected = hostLevel;
        }
        port::platform::SharedLevel mine;
        bool onPlayerLevel = s_scene && port::platform::currentSharedLevel(&mine);
        if (!sessionInfo().host && s_scene && s_remoteLevel >= 0 &&
            (s_remoteLevel != currentLevel() || (onPlayerLevel && !hostLevel))) {
            s_followLevel = s_remoteLevel;
            s_followNewMap = onPlayerLevel && !hostLevel;
        }
        log("соперник на уровне %d%s", s_remoteLevel, hostLevel ? " (уровень игрока)" : "");
        if (wasLevel >= 0 && s_remoteLevel < 0 && !loading) s_peerMenuAt = nowMs() + kMenuNoticeMs;
        if (s_remoteLevel >= 0) s_peerMenuAt = 0;
    } else if (type == kLevel) {
        if (sessionInfo().host) return;
        receivedLevel(in);
        port::platform::SharedLevel mine;
        bool onPlayerLevel = s_scene && port::platform::currentSharedLevel(&mine);
        if (s_scene && s_peerLevel.known && (!onPlayerLevel || mine.hash != s_peerLevel.info.hash)) {
            s_followLevel = s_remoteLevel;
            s_followNewMap = true;
        }
    } else if (type == kLevelPart) {
        if (!sessionInfo().host) receivedLevelPart(in, r.data);
    } else if (type == kChoice) {
        Browsing b;
        b.race = in.u8() != 0;
        b.level = int32_t(in.u32());
        b.timeLimit = int32_t(in.u32());
        b.combat = in.u8() != 0;
        b.playerLevel = in.str();
        if (in.ok && !sessionInfo().host) s_browseGot = b;
    } else if (type == kLevelState) {
        uint32_t hash = in.u32();
        int stage = in.u8();
        size_t got = in.u32(), total = in.u32();
        port::platform::SharedLevel mine;
        if (in.ok && sessionInfo().host && port::platform::currentSharedLevel(&mine) && uint32_t(mine.hash) == hash) {
            if (stage != s_guestMap.stage)
                log("соперник: карта уровня — %s", stage == kMapServer ? "скачивает с сервера" : stage == kMapFile ? "получает файлом"
                                                   : stage == kMapReady ? "готова" : "не вышло");
            s_guestMap.stage = stage;
            s_guestMap.got = got;
            s_guestMap.total = total;
            s_guestMap.at = nowMs();
        }
    } else if (type == kLevelNeed) {
        port::platform::SharedLevel shared;
        if (sessionInfo().host && port::platform::currentSharedLevel(&shared)) sendLevelParts(shared);
    } else if (type == kReady) {
        s_remoteReadyLevel = int32_t(in.u32());
        s_remoteRacing = false;
        if (!sessionInfo().host && s_scene && s_remoteReadyLevel == currentLevel() && s_finished)
            s_followRestart = true;
        if (s_scene && (rivalsMode() || s_together) && s_remoteReadyLevel == currentLevel() &&
            s_scene->getStateBehindMenu() == SceneGame::STATE_GAME)
            s_followRestart = true;
        log("соперник ждёт на старте уровня %d", s_remoteReadyLevel);
        if (!s_holding) notice(TEXT_PEER_READY);
    } else if (type == kGo) {
        if (s_holding && !s_goAt) s_goAt = nowMs();
    } else if (type == kState) {
        Frame f;
        uint32_t time = in.u32();
        int level = int32_t(in.u32());
        f.position.x = in.f32(), f.position.y = in.f32(), f.position.z = in.f32();
        f.visual = int16_t(in.u16());
        f.blend = in.f32();
        int bags = in.u16();
        uint8_t flags = in.u8();
        if (!in.ok) return;
        s_remoteLevel = level;
        s_remoteBags = bags;
        s_remoteAttacking = (flags & 1) != 0;
        s_remoteImmune = (flags & 2) != 0;
        f.flags = flags;
        long long sent = time;
        if (!s_frames.empty() && sent <= s_frames.back().time) return;
        long long offset = nowMs() - sent;
        if (!s_haveOffset || offset < s_offset) s_offset = offset, s_haveOffset = true;
        else s_offset += 1;
        f.time = sent;
        s_frames.push_back(f);
        if (s_frames.size() > 200) s_frames.pop_front();
    } else if (type == kSkin) {
        receivedSkin(in);
    } else if (type == kSkinPart) {
        receivedSkinPart(in, r.data);
    } else if (type == kBag || type == kBagDeny) {
        int index = in.u16();
        if (!in.ok) return;
        if (type == kBag) {
            remoteTookBag(index);
        } else if (s_myBags.erase(index) && myScreen()) {
            myScreen()->decNumCollectablesCollected();
            log("сумка %d — хоста, мне не засчитана", index);
        }
    } else if (type == kRivalHit) {
        MathVector from, seen;
        from.x = in.f32(), from.y = in.f32(), from.z = in.f32();
        seen.x = in.f32(), seen.y = in.f32(), seen.z = in.f32();
        GameScreen* screen = myScreen();
        GameObjectPlayer* me = screen ? screen->getPlayerObject() : nullptr;
        if (in.ok && me)
            log("соперник ударил: у него я был в %.2f, на деле — в %.2f (он сам в %.2f)", seen.x, me->m_position.x, from.x);
        if (remoteHitMe() && s_remote) s_remote->catchUp();
    } else if (type == kGift) {
        if (myScreen() && rivalsMode()) myScreen()->incNumCollectablesCollected();
    } else if (type == kTimer) {
        int ms = int32_t(in.u32());
        if (!in.ok) return;
        s_hostTime = ms;
        s_hostTimeAt = nowMs();
    } else if (type == kEnd) {
        s_endReceived = true;
    } else if (type == kQuit) {
        int level = int32_t(in.u32());
        if (in.ok && s_scene && level == currentLevel() && s_myFinish < 0 && (s_together || s_holding)) {
            s_remoteLeft = true;
            s_quitAt = nowMs() + kQuitAfterMs;
            s_closeOnLeave = true;
            topMessage(replaced(gameText(TEXT_LEFT), "{name}", peerName()));
            log("соперник вышел из заезда — выхожу следом");
        }
    } else if (type == kEnemy) {
        int level = int32_t(in.u32());
        int id = int32_t(in.u32());
        EnemyChange c;
        c.brain = in.u8();
        c.anim = in.u8();
        c.position.x = in.f32(), c.position.y = in.f32(), c.position.z = in.f32();
        c.velocity.x = in.f32(), c.velocity.y = in.f32(), c.velocity.z = in.f32();
        c.facing = in.f32();
        if (!in.ok || !s_scene || !s_together || s_holding || level != currentLevel()) return;
        GameObjectPolice* cop = findEnemy(id);
        if (!cop) return;
        if (enemyOut(c.brain)) s_enemiesOut[id] = c;
        applyEnemy(cop, c);
        log("противник %d %s соперником", id, enemyText(c.brain));
    } else if (type == kEvent) {
        uint8_t kind = in.u8();
        int raceTime = int32_t(in.u32());
        if (!in.ok) return;
        const char* names[] = {"?", "прошёл чекпоинт", "финишировал", "ударил"};
        log("соперник %s (%s с)", names[kind <= kHit ? kind : 0], secondsText(raceTime).c_str());
        if (kind == kFinish && s_together && s_remoteRacing && s_remoteFinish < 0) {
            s_remoteFinish = raceTime;
            if (s_myFinish < 0)
                topMessage(replaced(replaced(gameText(TEXT_FINISHED), "{name}", peerName()), "{time}",
                                    millisText(raceTime)));
        }
    }
}

void sendLevel(uint8_t type, int level) {
    if (!s_peer) return;
    Writer w;
    w.u8(type);
    w.u8(kVersion);
    w.u32(uint32_t(level));
    send(s_peer, w.b.data(), w.b.size(), true);
}

void start() {
    s_started = true;
    std::string error;
    if (const char* port = getenv("MEDECOMP_NET_HOST")) {
        if (host(uint16_t(atoi(port)), &error)) {
            sessionUseConnection(0, true);
            log("жду соперника на порту %s", port);
        } else {
            log("%s", error.c_str());
        }
    } else if (const char* address = getenv("MEDECOMP_NET_CONNECT")) {
        if (Peer peer = connect(address, &error)) {
            sessionUseConnection(peer, false);
            log("подключаюсь к %s", address);
        } else {
            log("%s", error.c_str());
        }
    }
}

}

int remoteLevel() { return s_peer ? s_remoteLevel : -1; }

bool remoteLocator(MathVector* at) {
    if (!s_peer || !s_remote || !s_remote->shown()) return false;
    *at = s_remote->m_position;
    return true;
}

void raceUserRestart() {
    if (!s_peer || !s_scene || !s_together || s_finished) return;
    s_remoteRacing = false;
    sendLevel(kReady, currentLevel());
    log("заново — и у соперника");
}

void raceUserQuit() {
    if (!s_peer || !s_scene || s_myFinish >= 0 || !(s_together || s_holding)) return;
    sendLevel(kQuit, currentLevel());
    s_closeOnLeave = true;
    log("вышел из заезда — соперник тоже");
}

bool raceOpponent() { return s_peer != 0; }

bool raceNoPause() { return s_noPause; }

void raceBrowsing(bool race, const Level* level, int timeLimit, bool combat, const char* playerLevel) {
    if (!s_peer || !sessionInfo().host) return;
    Browsing b;
    LevelData* levels = AppEngine::getLevelData();
    for (int i = 0; level && i < levels->getLevelNum() && b.level < 0; i++)
        if (levels->getLevel(i) == level) b.level = i;
    if (b.level >= 0) {
        b.race = race;
        b.timeLimit = race ? 0 : timeLimit;
        b.combat = combat;
        b.playerLevel = playerLevel ? playerLevel : "";
    }
    if (s_browseSentAny && b == s_browseSent) return;
    s_browseSent = b;
    s_browseSentAny = true;
    Writer w;
    w.u8(kChoice);
    w.u8(kVersion);
    w.u8(b.race ? 1 : 0);
    w.u32(uint32_t(b.level));
    w.u32(uint32_t(b.timeLimit));
    w.u8(b.combat ? 1 : 0);
    w.str(b.playerLevel);
    send(s_peer, w.b.data(), w.b.size(), true);
}

bool raceGettingLevel(int* percent, bool* failed) {
    if (!s_peer || sessionInfo().host || !s_peerLevel.expected || s_peerLevel.ready) return false;
    *failed = s_peerLevel.stage == kMapFailed;
    *percent = s_peerLevel.stage == kMapFile && s_peerLevel.size ? int(s_peerLevel.got * 100 / s_peerLevel.size) : -1;
    return true;
}

bool guestGettingMap(long long now) {
    return (s_guestMap.stage == kMapServer || s_guestMap.stage == kMapFile) && now - s_guestMap.at < kTransferQuietMs;
}

bool raceHostBrowsing(HostBrowsing* out) {
    if (!s_peer || sessionInfo().host || s_browseGot.level < 0) return false;
    *out = s_browseGot;
    return true;
}
bool raceTogether() { return s_together; }
bool raceIsHost() { return sessionInfo().host; }

bool raceHostChooses() {
    int mode = AppEngine::getLevelData()->getGameMode();
    return s_peer && sessionInfo().host &&
           (mode == LevelData::GAME_MODE_SPEEDRUN || mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED);
}

void raceChooseNext() {
    s_chooseNext = true;
    log("выбираю, что дальше");
}

bool takeChooseNext() {
    bool was = s_chooseNext && s_peer;
    s_chooseNext = false;
    return was;
}

bool raceChoiceToJoin(RaceChoice* out) {
    if (!s_peer) return false;
    int level = s_joinLevel >= 0 ? s_joinLevel : s_remoteReadyLevel;
    if (level < 0) return false;
    if (s_peerLevel.expected && !s_peerLevel.ready) return false;
    out->level = level;
    out->mode = s_remoteMode;
    out->timeLimit = s_remoteTimeLimit;
    out->combat = s_remoteCombat;
    out->playerLevel = s_peerLevel.expected ? s_peerLevel.localId : std::string();
    return true;
}

void raceApplyChoice(const RaceChoice& choice) {
    LevelData* levels = AppEngine::getLevelData();
    int level = choice.level;
    if (choice.playerLevel.empty()) {
        port::platform::clearPlayerLevel();
    } else {
        int base = -1;
        std::string error;
        if (port::platform::startPlayerLevelTogether(choice.playerLevel, &base, &error)) {
            level = base;
        } else {
            log("уровень игрока %s: %s", choice.playerLevel.c_str(), error.c_str());
            port::platform::clearPlayerLevel();
        }
    }
    levels->setCurrentLevelByIndex(static_cast<LevelData::GameMode>(choice.mode), level);
    if (choice.mode == LevelData::GAME_MODE_MULTIPLAYER_TIMED) levels->setLevelTimeLimitMillis(choice.timeLimit);
    levels->setMultiplayerCombatAllowed(choice.combat);
}

bool shirtForGuest(const port::platform::skins::Look& look, bool mine) {
    if (!s_peer || !s_peerSkinKnown) return false;
    port::platform::skins::Look other = mine ? peerLook() : myLook();
    if (!port::platform::skins::plainFaith(look) || !port::platform::skins::plainFaith(other)) return false;
    return mine ? !sessionInfo().host : sessionInfo().host;
}

bool rivalsNetwork() { return s_peer != 0 && AppEngine::getLevelData()->isMultiplayerMode(); }
int rivalsRemoteBags() { return s_peer ? s_remoteBags : 0; }

void rivalsBagTaken(GameObject* bag) {
    if (!s_scene || !rivalsNetwork()) return;
    int index = s_scene->getMap().getCollectableList().indexOf(bag);
    if (index < 0) return;
    s_myBags.insert(index);
    sendBag(kBag, index);
    log("взял сумку %d", index);
}

void enemyChanged(GameObjectPolice* cop) {
    int brain = cop->m_brainState;
    bool knocked = brain == GameObjectPolice::BRAIN_KNOCKED_DOWN || brain == GameObjectPolice::BRAIN_MELEED_FROM_PRONE;
    if (!s_peer || !s_scene || !s_together || s_holding || (!knocked && !enemyOut(brain))) return;
    int id = cop->getID();
    EnemyChange c{brain, cop->m_animState, cop->m_position, cop->m_velocity, cop->m_facing};
    if (enemyOut(brain)) s_enemiesOut[id] = c;
    Writer w;
    w.u8(kEnemy);
    w.u8(kVersion);
    w.u32(uint32_t(currentLevel()));
    w.u32(uint32_t(id));
    w.u8(uint8_t(c.brain));
    w.u8(uint8_t(c.anim));
    w.f32(c.position.x), w.f32(c.position.y), w.f32(c.position.z);
    w.f32(c.velocity.x), w.f32(c.velocity.y), w.f32(c.velocity.z);
    w.f32(c.facing);
    send(s_peer, w.b.data(), w.b.size(), true);
    log("противник %d %s", id, enemyText(brain));
}

void enemyAimBegin(GameObjectPolice* cop) {
    GameObjectPlayer* player = cop->m_player;
    if (!s_scene || !s_remote || !s_remote->shown() || !player || !cop->isActive()) return;
    const MathVector& remote = s_remote->m_position;
    if (!enemyNotices(cop, remote)) return;
    if (cop->getDistanceToSq(player) <= cop->getDistanceToSq(s_remote) && enemyNotices(cop, player->m_position)) return;
    s_aimPlayer = player;
    s_aimSaved = player->m_position;
    player->m_position = remote;
}

void enemyAimEnd() {
    if (!s_aimPlayer) return;
    s_aimPlayer->m_position = s_aimSaved;
    s_aimPlayer = nullptr;
}

GameObject* enemyBullet(MEdgeMap* map, float x, float y, float z, const MathVector& velocity) {
    return s_aimPlayer ? new RemoteBullet(map, x, y, z, velocity) : nullptr;
}

void rivalsGiveBag() {
    if (rivalsNetwork()) sendPlain(kGift);
}

bool rivalsSyncTime(int* raceTime) {
    if (!s_peer || sessionInfo().host || s_hostTime < 0) return false;
    long long left = s_hostTime - (nowMs() - s_hostTimeAt);
    *raceTime = int(left < 0 ? 0 : left);
    return true;
}

void raceJoined() { s_joinLevel = -1; }

bool raceHoldStart() {
    int mode = AppEngine::getLevelData()->getGameMode();
    if ((mode != LevelData::GAME_MODE_SPEEDRUN && mode != LevelData::GAME_MODE_MULTIPLAYER_TIMED) || !s_scene)
        return false;
    long long now = nowMs();
    int level = currentLevel();
    if (!s_holding) {
        bool remoteMidRun = s_remoteRacing && s_remoteFinish < 0 && s_remoteLevel == level && !rivalsMode();
        if (!s_peer || remoteMidRun) {
            startRun(false);
            return false;
        }
        s_holding = true;
        s_holdSince = now;
        s_goAt = 0;
        if (s_guestMap.stage == kMapFailed) s_guestMap.at = now;
        sendLevel(kReady, level);
        log("жду соперника на старте");
    }
    if (sessionInfo().host && guestGettingMap(now)) s_holdSince = now;
    bool mapFailed = sessionInfo().host && s_guestMap.stage == kMapFailed && now - s_guestMap.at > kMapFailShowMs;
    if (!s_peer || now - s_holdSince > kHoldMs || mapFailed) {
        log("старт без соперника");
        if (s_peer && !mapFailed) notice(TEXT_ALONE);
        startRun(false);
        return false;
    }
    if (sessionInfo().host && !s_goAt && s_remoteReadyLevel == level) {
        sendLevel(kGo, level);
        s_goAt = now + ping(s_peer) / 2;
    }
    if (s_goAt && now >= s_goAt) {
        log("старт вместе с соперником");
        startRun(true);
        return false;
    }
    return true;
}

const int kTextYou = 0x9bb;
const int kFontLabel = 8;
const int kAnchorLabel = 34;
const float kLabelAboveFeet = 2.05f;
const float kLabelMaxDepth = 45.0f;
const int kPointer = 6;
const int kLabelPadX = 8, kLabelPadY = 3;
const int kLabelShade = 140;

bool toScreen(GameScreen* screen, const MathVector& p, int* x, int* y) {
    GameCamera* camera = screen->m_camera;
    float ex = camera->getLookFromX(), ey = camera->getLookFromY(), ez = camera->getLookFromZ();
    float fx = camera->getLookAtX() - ex, fy = camera->getLookAtY() - ey, fz = camera->getLookAtZ() - ez;
    float len = sqrtf(fx * fx + fy * fy + fz * fz);
    if (len < 1e-4f) return false;
    fx /= len, fy /= len, fz /= len;
    float rx = -fz, rz = fx;
    float rlen = sqrtf(rx * rx + rz * rz);
    if (rlen < 1e-4f) return false;
    rx /= rlen, rz /= rlen;
    float ux = -rz * fy, uy = rz * fx - rx * fz, uz = rx * fy;
    float dx = p.x - ex, dy = p.y - ey, dz = p.z - ez;
    float depth = dx * fx + dy * fy + dz * fz;
    if (depth < camera->getNearClip() || depth > kLabelMaxDepth) return false;
    int extra = medecomp_wide_extra();
    float width = float(screen->m_width + 2 * extra), height = float(screen->m_height);
    float tanH = tanf(medecomp_wide_fov(camera->getFOV()) * 0.00872664626f);
    float tanV = tanH * height / width;
    float sx = 0.5f + 0.5f * (dx * rx + dz * rz) / (depth * tanH);
    float sy = 0.5f - 0.5f * (dx * ux + dy * uy + dz * uz) / (depth * tanV);
    if (sx < 0.0f || sx > 1.0f || sy < 0.0f || sy > 1.0f) return false;
    *x = int(width * sx) - extra;
    *y = int(height * sy);
    return true;
}

struct Label {
    bool shown = false;
    int x = 0, y = 0;
    int lift = 0;
};

Label placeLabel(GameScreen* screen, const MathVector& feet) {
    Label l;
    MathVector head(feet.x, feet.y + kLabelAboveFeet, feet.z);
    l.shown = toScreen(screen, head, &l.x, &l.y);
    return l;
}

void drawLabel(midp::Graphics* g, const Label& l, midp::String* text, bool mine) {
    if (!l.shown) return;
    TextManager* tm = AppEngine::getCanvas()->getTextManager();
    int w = tm->getStringWidth(text, kFontLabel) + 2 * kLabelPadX, h = tm->getLineHeight(kFontLabel) + 2 * kLabelPadY;
    int bottom = l.y - kPointer - l.lift;
    g->setColor(0, 0, 0, kLabelShade);
    g->fillRect(l.x - w / 2, bottom - h, w, h);
    if (mine) g->setColor(255, 255, 255);
    else g->setColor(255, 0, 0);
    g->fillRect(l.x - w / 2, bottom - 2, w, 2);
    if (l.lift == 0) g->fillTriangle(l.x - kPointer, l.y - kPointer, l.x + kPointer, l.y - kPointer, l.x, l.y);
    tm->drawString(g, text, kFontLabel, l.x, bottom - kLabelPadY, kAnchorLabel);
}

void renderLabels(midp::Graphics* g) {
    if (!s_scene || !s_remote || !s_remote->shown() || !s_peer) return;
    GameScreen* screen = s_scene->getGameScreen(0);
    GameObjectPlayer* player = screen ? screen->getPlayerObject() : nullptr;
    if (!player || screen->m_type != GameScreen::TYPE_SINGLE || screen->getState() >= GameScreen::SCREEN_MENU_MAIN)
        return;
    static std::string shownName;
    static int youLanguage = -1;
    static midp::ReferenceCountedPointer<midp::String> you(NULL), name(NULL);
    int language = AppEngine::getCanvas()->getTextManager()->getCurrentLanguage();
    if (!you || language != youLanguage) {
        youLanguage = language;
        you = fromUtf8(gameText(kTextYou));
    }
    std::string wanted = peerName();
    if (wanted.empty()) wanted = "?";
    if (wanted != shownName || !name) {
        shownName = wanted;
        name = fromUtf8(wanted);
    }
    Label theirs = placeLabel(screen, s_remote->m_position), mine = placeLabel(screen, player->m_position);
    TextManager* tm = AppEngine::getCanvas()->getTextManager();
    int lineH = tm->getLineHeight(kFontLabel) + 2 * kLabelPadY + 2;
    int halfW = (tm->getStringWidth(name, kFontLabel) + tm->getStringWidth(you, kFontLabel)) / 2 + 2 * kLabelPadX;
    if (theirs.shown && mine.shown && abs(theirs.x - mine.x) < halfW && abs(theirs.y - mine.y) < lineH)
        theirs.lift = lineH + (theirs.y - mine.y);
    drawLabel(g, theirs, name, false);
    drawLabel(g, mine, you, true);
}

const int kTextServerPing = 0x9dd;
const int kTextPeerPing = 0x9de;
const int kPingX = 16, kPingY = 16, kPingGap = 4;

struct PingPlate {
    std::string shown;
    midp::ReferenceCountedPointer<midp::String> text{NULL};

    int draw(midp::Graphics* g, const std::string& wanted, int y) {
        if (wanted.empty()) return y;
        if (wanted != shown || !text) {
            shown = wanted;
            text = fromUtf8(wanted);
        }
        TextManager* tm = AppEngine::getCanvas()->getTextManager();
        int w = tm->getStringWidth(text, kFontLabel) + 2 * kLabelPadX, h = tm->getLineHeight(kFontLabel) + 2 * kLabelPadY;
        g->setColor(0, 0, 0, kLabelShade);
        g->fillRect(kPingX, y, w, h);
        tm->drawString(g, text, kFontLabel, kPingX + w / 2, y + h - kLabelPadY, kAnchorLabel);
        return y + h + kPingGap;
    }
};

void renderPing(midp::Graphics* g) {
    if (!s_peer) return;
    static PingPlate server, peer;
    SessionInfo info = sessionInfo();
    int ms = ping(s_peer);
    std::string name = peerName();
    int y = server.draw(g, info.serverPing < 0 ? std::string()
                                               : replaced(gameText(kTextServerPing), "{ms}", std::to_string(info.serverPing)),
                        kPingY);
    peer.draw(g, ms < 0 ? std::string()
                        : replaced(replaced(gameText(kTextPeerPing), "{name}", name.empty() ? "?" : name), "{ms}",
                                   std::to_string(ms)),
              y);
}

void renderRace(midp::Graphics* g) {
    renderLabels(g);
    renderPing(g);
    if (!s_holding) return;
    AppEngine* app = AppEngine::getCanvas();
    TextManager* text = app->getTextManager();
    static std::string shown;
    static midp::ReferenceCountedPointer<midp::String> line(NULL), seconds(NULL);
    std::string wanted = replaced(gameText(TEXT_WAITING), "{name}", peerName());
    if (wanted != shown || !line) {
        shown = wanted;
        line = fromUtf8(wanted);
    }
    long long now = nowMs(), left = (kHoldMs - (now - s_holdSince) + 999) / 1000;
    std::string note = std::to_string(left < 0 ? 0 : left);
    if (sessionInfo().host && guestGettingMap(now)) {
        note = replaced(gameText(TEXT_PEER_GETTING_MAP), "{name}", peerName());
        if (s_guestMap.stage == kMapFile && s_guestMap.total)
            note += " " + std::to_string(s_guestMap.got * 100 / s_guestMap.total) + "%";
    } else if (sessionInfo().host && s_guestMap.stage == kMapFailed) {
        note = replaced(gameText(TEXT_PEER_MAP_FAILED), "{name}", peerName());
    }
    seconds = fromUtf8(note);
    int x = app->getWidth() / 2, y = kWaitingY;
    text->drawString(g, line, FONT_HEADING, x, y, ANCHOR_CENTER_BASELINE);
    text->drawString(g, seconds, FONT_NOTE, x, y + text->getLineHeight(FONT_NOTE) + 8, ANCHOR_CENTER_BASELINE);
}

void renderRivalsNote(midp::Graphics* g, int centerX, int baselineY) {
    if (!s_peer || sessionInfo().host) return;
    std::string name = peerName();
    static std::string shown;
    static midp::ReferenceCountedPointer<midp::String> note(NULL);
    std::string wanted = replaced(gameText(TEXT_HOST_DECIDES), "{name}", name.empty() ? "?" : name);
    if (wanted != shown || !note) {
        shown = wanted;
        note = fromUtf8(wanted);
    }
    AppEngine::getCanvas()->getTextManager()->drawString(g, note, FONT_NOTE, centerX, baselineY, ANCHOR_CENTER_BASELINE);
}

void renderRaceResult(midp::Graphics* g, int labelX, int timeX, int rowY, int centerX, int headlineY) {
    if (!s_together) return;
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    std::string name = peerName();
    if (name.empty()) name = "?";
    std::string time = s_remoteFinish >= 0 ? millisText(s_remoteFinish) : "--:--.--";
    std::string headline;
    int font = FONT_HEADING;
    bool behind = s_myFinish >= 0 && (s_remoteLeft || !s_peer || nowMs() - s_myFinishAt > kWinAfterMs);
    if (s_myFinish >= 0 && s_remoteFinish >= 0) {
        headline = s_myFinish < s_remoteFinish   ? gameText(TEXT_YOU_WIN)
                   : s_myFinish > s_remoteFinish ? replaced(gameText(TEXT_PEER_WINS), "{name}", name)
                                                 : gameText(TEXT_DRAW);
    } else if (behind) {
        headline = gameText(TEXT_YOU_WIN);
    }
    std::string note;
    if (s_remoteFinish < 0)
        note = replaced(gameText(s_remoteLeft || !s_peer ? TEXT_LEFT : TEXT_RUNNING), "{name}", name);
    else if (!sessionInfo().host && s_peer)
        note = replaced(gameText(TEXT_HOST_DECIDES), "{name}", name);
    static std::string shown;
    static midp::ReferenceCountedPointer<midp::String> label(NULL), timeText(NULL), headlineText(NULL), noteText(NULL);
    std::string key = name + "\n" + time + "\n" + headline + "\n" + note;
    if (key != shown) {
        shown = key;
        label = fromUtf8(name + ":");
        timeText = fromUtf8(time);
        headlineText = headline.empty() ? nullptr : fromUtf8(headline);
        noteText = note.empty() ? nullptr : fromUtf8(note);
    }
    text->drawString(g, label, FONT_ROW_LABEL, labelX, rowY, ANCHOR_RIGHT_BASELINE);
    text->drawString(g, timeText, FONT_ROW_TIME, timeX, rowY, ANCHOR_LEFT_BASELINE);
    int y = headlineY;
    if (noteText) {
        text->drawString(g, noteText, FONT_NOTE, centerX, y, ANCHOR_CENTER_BASELINE);
        y -= text->getLineHeight(FONT_NOTE) + 4;
    }
    if (headlineText) text->drawString(g, headlineText, font, centerX, y, ANCHOR_CENTER_BASELINE);
}

void raceFrame(int dt) {
    (void)dt;
    if (!s_started && (getenv("MEDECOMP_NET_HOST") || getenv("MEDECOMP_NET_CONNECT"))) start();
    sessionFrame();
    static SessionState lastState = SessionState::Idle;
    SessionInfo info = sessionInfo();
    if (info.state != lastState) {
        if (info.state == SessionState::WaitingGuest && info.host && !info.code.empty())
            notice(replaced(gameText(TEXT_ROOM_OPEN), "{code}", info.code));
        lastState = info.state;
    }
    s_noPause = s_peer || (info.state != SessionState::Idle && info.state != SessionState::Failed);
    if (sessionInfo().state == SessionState::Idle && !s_peer) return;
    Received r;
    while (sessionPoll(&r)) {
        if (r.event == Event::Connected) {
            s_peer = r.peer;
            log("соперник подключился (%s)", r.text.c_str());
            notice(TEXT_JOINED);
            sendHello();
            s_browseSentAny = false;
            s_browseGot = Browsing();
            s_skinSent = false;
            s_peerSkinKnown = false;
            s_peerFiles[0] = s_peerFiles[1] = PeerFile();
            s_peerArrived = 0;
            sendSkin();
        } else if (r.event == Event::Disconnected) {
            if (r.peer == s_peer) s_peer = 0;
            s_browseGot = Browsing();
            s_guestMap = GuestMap();
            s_partQueue = PartQueue();
            if (s_together && s_remoteFinish < 0) s_remoteLeft = true;
            s_remoteReadyLevel = -1;
            s_joinLevel = -1;
            s_frames.clear();
            s_haveOffset = false;
            s_peerLevel = PeerLevel();
            log("соперник отключился (%s)", r.text.c_str());
            s_peerMenuAt = 0;
            notice(r.text == kNormalClose ? TEXT_PEER_QUIT : TEXT_LOST);
        } else if (r.peer == s_peer) {
            receive(r);
        }
    }
    levelFetchFrame();
    if (s_peerMenuAt && nowMs() >= s_peerMenuAt) {
        s_peerMenuAt = 0;
        if (s_peer && s_remoteLevel < 0) notice(sessionInfo().host ? TEXT_PEER_IN_MENU : TEXT_HOST_DECIDES);
    }
    levelPartsFrame();
    if (s_peerArrived) {
        if (s_remote) {
            s_remote->redress(s_peerArrived);
            log("вид соперника: переодет");
        }
        s_peerArrived = 0;
    }

    AppEngine* app = AppEngine::getCanvas();
    SceneGame* scene = app && app->m_sceneId == kSceneGame ? app->getSceneGame() : nullptr;
    if (scene && (scene->m_loadingStep < 13 || scene->m_gameScreens.m_length <= 0)) scene = nullptr;
    if (scene != s_scene) {
        s_scene = scene;
        s_remote = nullptr;
        s_lastVisual = -2;
        s_finished = false;
        s_holding = false;
        s_together = false;
        s_myFinish = -1;
        s_remoteFinish = -1;
        s_followLevel = -1;
        s_followNewMap = false;
        s_followRestart = false;
        s_myBags.clear();
        s_enemiesOut.clear();
        s_remoteBags = 0;
        s_hostTime = -1;
        s_endReceived = false;
        s_quitAt = 0;
        if (s_closeOnLeave && s_peer) {
            log("вышли из заезда по QUIT — сеанс закрыт");
            sessionClose();
            s_peer = 0;
        }
        s_closeOnLeave = false;
        int mode = scene ? AppEngine::getLevelData()->getGameMode() : -1;
        if (s_peer && (mode == LevelData::GAME_MODE_STORY || mode == LevelData::GAME_MODE_CHALLENGE)) {
            log("одиночная игра — сеанс закрыт");
            sessionClose();
            s_peer = 0;
        }
        if (s_peer) sendHello();
    }
    if (!scene) return;
    GameObjectPlayer* player = scene->getGameScreen(0)->getPlayerObject();
    if (!player) return;
    int level = currentLevel();

    for (const auto& e : s_enemiesOut) {
        GameObjectPolice* cop = findEnemy(e.first);
        if (!cop || !cop->isActive()) continue;
        if (e.second.brain == GameObjectPolice::BRAIN_FALLEN) {
            cop->m_brainState = GameObjectPolice::BRAIN_FALLEN;
            cop->stateTransition(GameObjectNPC::NPC_STATE_DISABLED);
            cop->deactivate();
        } else {
            applyEnemy(cop, e.second);
        }
    }

    long long now = nowMs();
    int visual = player->getVisual();
    if (s_peer && (now - s_lastSent >= kSendEveryMs || visual != s_lastVisual)) {
        Writer w;
        w.u8(kState);
        w.u8(kVersion);
        w.u32(uint32_t(now));
        w.u32(uint32_t(level));
        w.f32(player->m_position.x), w.f32(player->m_position.y), w.f32(player->m_position.z);
        w.u16(uint16_t(int16_t(visual)));
        w.f32(player->get3WayAnimationBlendWeights());
        w.u16(uint16_t(player->getNumCollectablesCollected()));
        w.u8(uint8_t((player->isAttacking() ? 1 : 0) | (player->isImmuneToOtherPlayer() ? 2 : 0)));
        send(s_peer, w.b.data(), w.b.size(), false);
        s_lastSent = now;
        s_lastVisual = visual;
    }
    const MathVector& checkpoint = player->m_checkpointPosition;
    if (checkpoint.x != s_lastCheckpoint.x || checkpoint.y != s_lastCheckpoint.y || checkpoint.z != s_lastCheckpoint.z) {
        s_lastCheckpoint = checkpoint;
        sendEvent(kCheckpoint, scene->m_raceTime, checkpoint);
    }
    int state = scene->getStateBehindMenu();
    bool finished = state == SceneGame::STATE_LEVEL_COMPLETE_SINGLE || state == SceneGame::STATE_LEVEL_COMPLETE_MULTIPLAYER;
    if (finished && !s_finished) {
        log("финиш: %s с", secondsText(scene->m_raceTime).c_str());
        sendEvent(kFinish, scene->m_raceTime, player->m_position);
        if (rivalsMode() && sessionInfo().host) sendPlain(kEnd);
        if (s_together && s_myFinish < 0) {
            s_myFinish = scene->m_raceTime;
            s_myFinishAt = now;
        }
    }
    s_finished = finished;
    if (rivalsMode() && s_peer) {
        if (sessionInfo().host && state == SceneGame::STATE_GAME && now - s_lastTimerSent >= kTimerEveryMs) {
            Writer w;
            w.u8(kTimer);
            w.u8(kVersion);
            w.u32(uint32_t(scene->m_raceTime));
            send(s_peer, w.b.data(), w.b.size(), false);
            s_lastTimerSent = now;
        }
        if (s_endReceived && !finished && state == SceneGame::STATE_GAME) {
            log("хост закончил игру");
            scene->completeLevel();
        }
        s_endReceived = false;
    }
    if (s_quitAt && nowMs() >= s_quitAt) {
        s_quitAt = 0;
        if (!finished) {
            scene->closeMenuOverGame();
            scene->stateTransition(SceneGame::STATE_EXIT_LEVEL);
            app->startFadeOut(true, 0xffffff);
            return;
        }
    }
    if (s_followRestart && (rivalsMode() || s_together) && state == SceneGame::STATE_GAME) {
        log("соперник начал заново — я тоже");
        topMessage(replaced(gameText(TEXT_RESTARTED), "{name}", peerName()));
        scene->closeMenuOverGame();
        scene->setExitState(SceneGame::STATE_RESTART_LEVEL);
        scene->stateTransition(SceneGame::STATE_EXIT_FADE);
        s_followRestart = false;
        return;
    }
    bool atEnd = state == SceneGame::STATE_LEVEL_COMPLETE_SINGLE || state == SceneGame::STATE_LEVEL_COMPLETE_MULTIPLAYER;
    bool running = state == SceneGame::STATE_GAME || state == SceneGame::STATE_INTRO_CAM || state == SceneGame::STATE_RACE_START;
    bool newMap = s_followLevel >= 0 && (s_followLevel != level || s_followNewMap);
    if ((s_followLevel >= 0 || s_followRestart) && (atEnd || (newMap && running))) {
        if (newMap && s_peerLevel.expected && !s_peerLevel.ready) return;
        scene->closeMenuOverGame();
        for (UI::Window* w : app->getWindowStore().m_windows) w->close(UI::RESULT_NONE);
        s_holding = false;
        if (newMap) {
            notice(TEXT_FOLLOWING);
            int next = s_followLevel;
            int base = -1;
            std::string error;
            if (!s_peerLevel.expected) {
                port::platform::clearPlayerLevel();
            } else if (port::platform::startPlayerLevelTogether(s_peerLevel.localId, &base, &error)) {
                next = base;
            } else {
                log("уровень игрока %s: %s", s_peerLevel.localId.c_str(), error.c_str());
                port::platform::clearPlayerLevel();
            }
            log("иду за хостом на уровень %d%s", next, s_peerLevel.expected ? " (уровень игрока)" : "");
            AppEngine::getLevelData()->setCurrentLevelByIndex(static_cast<LevelData::GameMode>(s_remoteMode), next);
            scene->m_gameActive = false;
            app->changeScene(kSceneGame);
        } else {
            log("хост начал заново — я тоже");
            scene->setExitState(SceneGame::STATE_RESTART_LEVEL);
            scene->stateTransition(SceneGame::STATE_EXIT_FADE);
        }
        s_followLevel = -1;
        s_followRestart = false;
        return;
    }

    if (!s_peer || s_remoteLevel != level) {
        if (s_remote) s_remote->setShown(false);
        return;
    }
    MEdgeMap& map = scene->getMap();
    if (!s_remote) {
        s_remote = new RemoteRunner(&map);
        midp::INCREF(static_cast<midp::ReferenceCounted*>(s_remote));
        log("соперник на этом уровне");
    }
    if (!map.m_updateObjects.contains(s_remote)) map.addObject(s_remote);
    if (combatChosen() && s_remote->shown() &&
        state == SceneGame::STATE_GAME && player->isAttacking() && !s_remoteImmune && now >= s_hitCooldownUntil &&
        fabsf(player->m_position.x - s_remote->m_position.x) < kHitReachX &&
        fabsf(player->m_position.y - s_remote->m_position.y) < kHitReachY) {
        Writer w;
        w.u8(kRivalHit);
        w.u8(kVersion);
        w.f32(player->m_position.x), w.f32(player->m_position.y), w.f32(player->m_position.z);
        w.f32(s_remote->m_position.x), w.f32(s_remote->m_position.y), w.f32(s_remote->m_position.z);
        send(s_peer, w.b.data(), w.b.size(), true);
        s_remote->predictKnockdown();
        hitSound();
        s_hitCooldownUntil = now + kHitCooldownMs;
        log("удар по сопернику: я в %.2f, он у меня в %.2f", player->m_position.x, s_remote->m_position.x);
    }
    static long long lastLog;
    if (verbose() >= 2 || (verbose() && now - lastLog >= 1000)) {
        lastLog = now;
        log("я (%.2f, %.2f, %.2f), соперник (%.2f, %.2f, %.2f) %s, кадров %zu, отставание %lld мс, пинг %d мс",
            player->m_position.x, player->m_position.y, player->m_position.z, s_remote->m_position.x,
            s_remote->m_position.y, s_remote->m_position.z, s_remote->shown() ? "виден" : "спрятан", s_frames.size(),
            s_frames.empty() ? -1LL : now - s_offset - s_frames.back().time, ping(s_peer));
    }
}

}
}
