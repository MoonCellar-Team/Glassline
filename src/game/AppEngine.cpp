#include <stdint.h>
#include "game/AppEngine.h"

#include <stdio.h>

#include "ea_online/ea/EASpywareManager.h"
#include "ea_online/Mayhem/Token.h"
#include "game/AchievementNotification.h"
#include "game/AnimPlayerQuad.h"
#include "game/AnimationManager.h"
#include "game/AppEngineThread.h"
#include "game/BGMusic.h"
#include "game/GameCenter.h"
#include "game/GameCommon.h"
#include "game/Level.h"
#include "game/LoadingScreen.h"
#include "game/LocaleManager.h"
#include "game/MediaPicker.h"
#include "game/MonkeyApp.h"
#include "game/QuadMesh.h"
#include "game/ReflectionMapShader.h"
#include "game/SceneGame.h"
#include "game/SceneMenu.h"
#include "game/SceneStartup.h"
#include "m3g/Appearance.h"
#include "m3g/Background.h"
#include "m3g/Camera.h"
#include "m3g/Graphics3D.h"
#include "m3g/Image2D.h"
#include "m3g/Texture2D.h"
#include "m3g/Transform.h"
#include "m3g/VertexBuffer.h"
#include "m3g/World.h"
#include "runtime/midp/Class.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/Runtime.h"
#include "runtime/midp/RuntimeIPhone.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#include "runtime/midp/Thread.h"
#include "sdk/CoreFoundation.h"

#ifdef MEDECOMP_PORT
extern "C" void medecomp_menu_focus_draw(midp::Graphics* g);
extern "C" bool medecomp_skin_preview(midp::Graphics* g);
extern "C" bool medecomp_level_preview(midp::Graphics* g);
#endif

static AppEngine* s_canvas;

const char* AppEngine::PENDING_CHALLENGES_FILENAME = "pendingChallenges";

static const int FADE_ANIM = 11;
static const int FADE_MESH = 0x57;
static const int POINTER_CAPACITY = 0x32;
static const float RAND_SCALE = 1.0f / 65536.0f;
static const float LOADING_SCROLL_SPEED = -1.0000000656873453e-05f;

AppEngine* AppEngine::getCanvas() {
    return s_canvas;
}

void AppEngine::createAppEngine(MonkeyApp* midlet) {
    s_canvas = new AppEngine(midlet);
}

m3g::Graphics3D* AppEngine::getGraphics3D() {
    return m_graphics3d;
}

ResourceManager& AppEngine::getResourceManager() {
    return m_resources;
}

TextManager* AppEngine::getTextManager() {
    return &m_text;
}

LevelData* AppEngine::getLevelData() {
    return &getCanvas()->m_levelData;
}

M3GAssets* AppEngine::getM3GAssets() {
    return &getCanvas()->m_m3gAssets;
}

UI::LoadingScreen* AppEngine::getLoadingScreen(int index) {
    return m_loadingScreens[index];
}

midp::MIDlet* AppEngine::getMIDlet() {
    return m_midlet;
}

SoundManager& AppEngine::getSoundManager() {
    return m_sound;
}

BGMusic* AppEngine::getBGMusic() {
    return m_bgMusic;
}

QuadManager& AppEngine::getQuadManager() {
    return m_quads;
}

float AppEngine::getLoadingScrollValue() const {
    return m_loadingScroll;
}

void AppEngine::setLoadingScrollValue(float value) {
    m_loadingScroll = value;
}

void AppEngine::setScrollingLoad(bool scrolling) {
    m_scrollingLoad = scrolling;
}

MediaPicker* AppEngine::getMediaPicker() {
    return m_mediaPicker;
}

UI::WindowStore& AppEngine::getWindowStore() {
    return m_windows;
}

ReflectionMapShader* AppEngine::getReflectionShader() const {
    return m_reflection;
}

bool AppEngine::IsMayhemSessionValid() const {
    if (m_mayhemId.length() == 0) {
        return false;
    }
    return m_mayhemToken->IsValid();
}

AnimationBlenderData* AppEngine::getAnimationBlenderData() {
    return &getCanvas()->m_animBlenderData;
}

GameObjectData* AppEngine::getGameObjectData() {
    return &getCanvas()->m_gameObjectData;
}

GameObjectRunnerData* AppEngine::getGameObjectRunnerData() {
    return &getCanvas()->m_runnerData;
}

AchievementData* AppEngine::getAchievementData() {
    return &getCanvas()->m_achievementData;
}

int AppEngine::getSaveFileError() {
    return m_saveFileError;
}

AnimationManager3D& AppEngine::getAnimationManager3D() {
    return m_animManager3D;
}

AnimPlayer* AppEngine::getLoadingRunAnimPlayer() {
    return &m_loadingRun;
}

AnimationManagerData& AppEngine::getAnimManData() {
    return m_animManData;
}

SceneGame* AppEngine::getSceneGame() {
    if (m_sceneId != 3) {
        return 0;
    }
    return static_cast<SceneGame*>(static_cast<Scene*>(m_scene));
}

SceneMenu* AppEngine::getSceneMenu() {
    if (m_sceneId != 2) {
        return 0;
    }
    return static_cast<SceneMenu*>(static_cast<Scene*>(m_scene));
}

void AppEngine::endGame() {
    m_running = false;
}

bool AppEngine::isPaused() {
    return m_paused;
}

void AppEngine::keyPressed(int) {}

void AppEngine::keyReleased(int) {}

bool AppEngine::isFading() {
    return m_fadeDone == 0;
}

void AppEngine::changeScene(int scene, int param) {
    m_nextScene = scene;
    m_nextSceneParam = param;
}

void AppEngine::changeScene(int scene) {
    changeScene(scene, -1);
}

void AppEngine::resetGameOptions() {
    *reinterpret_cast<unsigned*>(&m_tutorialBoxes) = 0x1010101;
}

int AppEngine::mayhemScoreToTime(int score, int levelIndex) {
    const Level* level = getLevelData()->getLevel(levelIndex);
    return level->getSpeedRunRequirementMillis(1) - score;
}

int AppEngine::timeToMayhemScore(int timeMs, int levelIndex) {
    const Level* level = getLevelData()->getLevel(levelIndex);
    return level->getSpeedRunRequirementMillis(1) - timeMs;
}

void AppEngine::freeLoadingScreens() {
    if (m_loadingScreens.m_length < 1) {
        return;
    }
    for (int i = 0; i < m_loadingScreens.m_length; i++) {
        if (m_loadingScreens[i] != 0) {
            delete m_loadingScreens[i];
        }
    }
}

void AppEngine::processPointerEvents() {
    if (m_scene == 0) {
        return;
    }
    int buffer = m_pointerBuffer;
    m_pointerBuffer = 1 - buffer;
    if (m_pointerCounts[buffer] > 0) {
        for (int i = 0; i < m_pointerCounts[buffer]; i++) {
            int type = m_pointerTypes[buffer][i];
            int x = m_pointerX[buffer][i];
            int y = m_pointerY[buffer][i];
            int id = m_pointerIds[buffer][i];
            if (type == 2) {
                m_scene->pointerReleased(x, y, id);
            } else if (type == 1) {
                m_scene->pointerDragged(x, y, id);
            } else if (type == 0) {
                int tap = m_pointerTaps[buffer][i];
                m_scene->pointerPressed(x, y, id, tap);
            }
        }
    }
    m_pointerCounts[buffer] = 0;
}

void AppEngine::addPointerEvent(int type, int x, int y, int id, int tapCount) {
    int buffer = m_pointerBuffer;
    int count = m_pointerCounts[buffer];
    if (count < POINTER_CAPACITY) {
        m_pointerCounts[buffer] = count + 1;
        m_pointerTypes[buffer][count] = type;
        m_pointerX[buffer][count] = x;
        m_pointerY[buffer][count] = y;
        m_pointerIds[buffer][count] = id;
        m_pointerTaps[buffer][count] = tapCount;
    }
}

void AppEngine::savePendingChallenges() {
    midp::ReferenceCountedPointer<midp::OutputStream> file(
        midp::OutputStream::getResourceAsStream(new midp::String(PENDING_CHALLENGES_FILENAME)));
    if (file == 0) {
        return;
    }
    midp::DataOutputStream out(file);
    out.writeInt(2);
    out.writeInt(static_cast<int>(m_pendingChallenges.size()));
    while (m_pendingChallenges.size() != 0) {
        PendingChallenge& item = m_pendingChallenges.front();
        midp::ReferenceCountedPointer<midp::String> id(new midp::String(item.m_id.c_str()));
        midp::ReferenceCountedPointer<midp::String> name(new midp::String(item.m_name.c_str()));
        out.writeUTF(id);
        out.writeUTF(name);
        m_pendingChallenges.pop();
    }
}

void AppEngine::loadPendingChallenges() {
    midp::ReferenceCountedPointer<midp::InputStream> file(
        midp::Class::getResourceAsStream(new midp::String(PENDING_CHALLENGES_FILENAME)));
    if (file == 0) {
        return;
    }
    midp::DataInputStream in(file);
    int version = in.readInt();
    if (version == 2) {
        int count = in.readInt();
        while (count > 0) {
            midp::ReferenceCountedPointer<midp::String> id(in.readUTF());
            midp::ReferenceCountedPointer<midp::String> name(in.readUTF());
            CFStringEncoding encoding = CFStringGetSystemEncoding();
            const char* idText = CFStringGetCStringPtr(id->getCFString(), encoding);
            encoding = CFStringGetSystemEncoding();
            const char* nameText = CFStringGetCStringPtr(name->getCFString(), encoding);
            m_pendingChallenges.push(PendingChallenge(idText, nameText));
            count = count - 1;
        }
    }
}

void AppEngine::receivedNotification(std::map<std::string, std::string> info) {
    std::string idKey("challengeId");
    std::string alertKey("alert");
    PendingChallenge challenge(info[idKey], info[alertKey]);
    m_pendingChallenges.push(challenge);
}

void AppEngine::saveGameOptions() {
    m_spywareEnabled = ea::EASpywareManager::getInstance()->isEnabled();
    midp::ReferenceCountedPointer<midp::OutputStream> file(
        midp::OutputStream::getResourceAsStream(new midp::String("gopt.bin")));
    if (file == 0) {
        return;
    }
    midp::DataOutputStream out(file);
    out.writeByte(4);
    out.writeBoolean(m_tutorialBoxes);
    out.writeBoolean(m_uploadScores);
    out.writeBoolean(m_unk8d6);
    out.writeBoolean(m_spywareEnabled);
    out.writeInt(m_text.getCurrentLanguage());
}

#ifdef MEDECOMP_PORT
static void applyLauncherLanguage(TextManager& text) {
    const char* code = medecomp_language();
    if (code == NULL)
        return;
    midp::ReferenceCountedPointer<midp::String> wanted(new midp::String(code));
    LocaleManager::StringList supported = LocaleManager::getInstance()->getSupportedLocales();
    for (int i = 0; i < supported.m_length; i++) {
        if (wanted->equals(supported[i])) {
            text.setCurrentLanguage(i);
            return;
        }
    }
}
#endif

void AppEngine::loadGameOptions() {
    midp::ReferenceCountedPointer<midp::InputStream> file(
        midp::Class::getResourceAsStream(new midp::String("gopt.bin")));
    if (file == 0) {
        resetGameOptions();
#ifdef MEDECOMP_PORT
        applyLauncherLanguage(m_text);
#endif
        saveGameOptions();
        return;
    }
    midp::DataInputStream in(file);
    if (in.available() < 9 || in.readByte() != 4) {
        resetGameOptions();
#ifdef MEDECOMP_PORT
        applyLauncherLanguage(m_text);
#endif
        saveGameOptions();
        return;
    }
    m_tutorialBoxes = in.readBoolean();
    m_uploadScores = in.readBoolean();
    m_unk8d6 = in.readBoolean();
    m_spywareEnabled = in.readBoolean();
    m_text.setCurrentLanguage(in.readInt());
#ifdef MEDECOMP_PORT
    applyLauncherLanguage(m_text);
#endif
}

void AppEngine::notifyAchievementComplete(Achievement* achievement) {
    m_achievementNote->addAchievement(achievement);
}

void AppEngine::updateLoading(int dt) {
    if (m_loadingTexture == 0) {
        return;
    }
    if (m_scrollingLoad) {
        m_loadingScroll = m_loadingScroll + static_cast<float>(dt) * LOADING_SCROLL_SPEED;
    }
    float scroll = m_loadingScroll;
    if (scroll < 0.0f) {
        do {
            scroll = scroll + 1.0f;
        } while (scroll < 0.0f);
        m_loadingScroll = scroll;
    }
    m_loadingTexture->setTranslation(0.0f, scroll, 0.0f);
}

void AppEngine::renderLoading(midp::Graphics* g) {
    int width = getWidth();
    int height = getHeight();
    if (m_loadingWorld != 0) {
        m3g::Graphics3D* g3d = getGraphics3D();
        g3d->bindTarget(g);
        g3d->setViewport(0, 0, width, height);
        g3d->render(m_loadingWorld);
        g3d->releaseTarget();
    }
}

void AppEngine::deinitLoadingScreen() {
    m_m3gAssets.freeCaches(0x10);
    m_loadingWorld->removeChild(m_loadingNode);
    m_loadingNode = 0;
    m_loadingAppearance = 0;
    m_loadingTexture = 0;
}

void AppEngine::initLoadingScreen(int screen) {
    UI::LoadingScreen* loading = getLoadingScreen(screen);
    midp::ReferenceCountedPointer<m3g::Image2D> image2d;
    int model = (int)reinterpret_cast<intptr_t>(loading);
    if (loading->getTotalHeight() < 0x200) {
#ifdef MEDECOMP_PORT
        const int scale = medecomp_text_scale();
        midp::ImageIPhone* page = midp::Image::createImage(0x100 * scale, 0x200 * scale);
        if (scale != 1)
            medecomp_cg_base_scale(page->m_context, static_cast<float>(scale));
        midp::ReferenceCountedPointer<midp::Image> image(page);
#else
        midp::ReferenceCountedPointer<midp::Image> image(midp::Image::createImage(0x100, 0x200));
#endif
        midp::ReferenceCountedPointer<midp::Graphics> graphics(image->getGraphics());
        graphics->setClip(0, 0, 0x100, 0x200);
        loading->render(graphics, 0, 0);
        image2d = new m3g::Image2D(99, image);
        int colour = loading->getColour();
        if (colour == 2) {
            model = 6;
        } else if (colour == 1) {
            model = 5;
        } else if (colour == 0) {
            model = 4;
        }
    } else {
#ifdef MEDECOMP_PORT
        const int scale = medecomp_text_scale();
        midp::ImageIPhone* page = midp::Image::createImage(0x100 * scale, 0x400 * scale);
        if (scale != 1)
            medecomp_cg_base_scale(page->m_context, static_cast<float>(scale));
        midp::ReferenceCountedPointer<midp::Image> image(page);
#else
        midp::ReferenceCountedPointer<midp::Image> image(midp::Image::createImage(0x100, 0x400));
#endif
        midp::ReferenceCountedPointer<midp::Graphics> graphics(image->getGraphics());
        graphics->setClip(0, 0, 0x100, 0x400);
        loading->render(graphics, 0, 0);
        image2d = new m3g::Image2D(99, image);
        int colour = loading->getColour();
        if (colour == 2) {
            model = 3;
        } else if (colour == 1) {
            model = 2;
        } else if (colour == 0) {
            model = 1;
        }
    }
    m3g::Node* node = m_m3gAssets.loadModel(model, 0x10);
    M3GAssets::addNode(m_loadingWorld, node);
    m_loadingNode = node;
    m3g::Node* found = static_cast<m3g::Node*>(m_loadingNode->find(0x78));
    found->getTransformTo(m_loadingWorld, m_loadingTransform);
    m_loadingTransform->postTranslate(0.0f, 0.005f, 0.0f);
    m_loadingTransform->postRotate(90.0f, 0.0f, 0.0f, 1.0f);
    m_loadingCamera->setTransform(m_loadingTransform);
    m_loadingAppearance = static_cast<m3g::Appearance*>(m_m3gAssets.loadTexturedAppearance(6, 0x10));
    m_loadingTexture = M3GAssets::createTexture2D(image2d, 0xd2, 0xd1, 0xe4);
    m_loadingTexture->setTranslation(0.0f, m_loadingScroll, 0.0f);
    m_loadingAppearance->setTexture(0, m_loadingTexture);
    m_loadingScroll = 1.6f;
    m_loadingTexture->setTranslation(0.0f, 1.6f, 0.0f);
    m_scrollingLoad = true;
}

void AppEngine::loadLoadingScreens() {
    midp::DataInputStream in(ResourceManager::loadBinaryFile(0xc9));
    int count = in.readInt();
    {
        midp::array<UI::LoadingScreen*> screens(count);
        m_loadingScreens = screens;
    }
    for (int i = 0; i < count; i++) {
        m_loadingScreens[i] = new UI::LoadingScreen(in);
    }
    in.close();
}

void AppEngine::loadLoadingAssets() {
    m_loadingWorld = new m3g::World();
    int height = getHeight();
    int width = getWidth();
    float aspect = static_cast<float>(height) / static_cast<float>(width);
    m_loadingCamera = new m3g::Camera();
#ifdef MEDECOMP_PORT
    m_loadingCamera->setPerspective(medecomp_wide_fov(54.0f), medecomp_wide_aspect(height), 0.01f, 2.0f);
#else
    m_loadingCamera->setPerspective(54.0f, aspect, 0.01f, 2.0f);
#endif
    m_loadingWorld->addChild(m_loadingCamera);
    m_loadingWorld->setActiveCamera(m_loadingCamera);
    m3g::Background* background = new m3g::Background();
    background->setColorClearEnable(true);
    background->setColor(-1);
    background->setDepthClearEnable(true);
    m_loadingWorld->setBackground(background);
    m_loadingTransform = new m3g::Transform();
    m_loadingScroll = 0.0f;
    loadLoadingScreens();
}

void AppEngine::loadAnimManager3D() {
    m_animManager3D.loadAnimFile(m_resources);
}

bool AppEngine::saveRMSAppSettings() {
    midp::OutputStream* stream =
        midp::OutputStream::getResourceAsStream(new midp::String("gamesett"));
    if (stream != 0) {
        midp::DataOutputStream out(stream);
        out.writeInt(0x3b);
        out.writeBoolean(m_unk7f0);
        out.writeFloat(m_sound.getVolumeSFX());
        out.writeFloat(m_sound.getVolumeMusic());
        out.writeUTF(m_osVersion);
    }
    return stream != 0;
}

void AppEngine::resetRMSAppSettings() {
    m_unk7f0 = true;
    m_sound.setVolumeSFX(1.0f);
    m_sound.setVolumeMusic(1.0f);
    if (m_osVersion != 0) {
        midp::DECREF(m_osVersion);
        m_osVersion = 0;
    }
    m_osVersion = static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->getOSVersion();
    midp::INCREF(m_osVersion);
    m_firstLaunch = true;
}

void AppEngine::loadRMSAppSettings() {
    midp::InputStream* stream = midp::Class::getResourceAsStream(new midp::String("gamesett"));
    if (stream != 0) {
        midp::DataInputStream in(stream);
        if (in.available() >= 0xd && in.readInt() == 0x3b) {
            m_unk7f0 = in.readBoolean();
            float sfx = in.readFloat();
            float music = in.readFloat();
            m_sound.setVolumeSFX(sfx);
            m_sound.setVolumeMusic(music);
            m_osVersion = in.readUTF();
            midp::INCREF(m_osVersion);
            if (!in.eofExceptionThrown()) {
                return;
            }
        }
    }
    resetRMSAppSettings();
    saveRMSAppSettings();
}

void AppEngine::loadSounds() {
    m_sound.loadData();
    m_sound.loadEvent(0x24);
    m_sound.loadEvent(0x23);
    m_sound.loadEvent(0x25);
}

float AppEngine::randPercentile() {
    return static_cast<float>(m_random.nextInt(0x10000)) * RAND_SCALE;
}

float AppEngine::randFloat(float min, float max) {
    float span = max - min;
    return span * static_cast<float>(m_random.nextInt(0x10000)) * RAND_SCALE + min;
}

int AppEngine::randPercent() {
    return m_random.nextInt(100);
}

int AppEngine::rand(int min, int max) {
    return m_random.nextInt((1 - min) + max) + min;
}

int AppEngine::rand(int n) {
    return m_random.nextInt(n);
}

void AppEngine::renderBgFade(midp::Graphics* g) {
    m_quads.render(g, 8);
}

bool AppEngine::isFadedIn() {
    if (m_fadeDone == 0) {
        return false;
    }
    return m_quads.getAnimPlayer(FADE_ANIM)->getCurrentKeyframe() == 1;
}

bool AppEngine::isFadedOut() {
    if (m_fadeDone == 0) {
        return false;
    }
    return m_quads.getAnimPlayer(FADE_ANIM)->getCurrentKeyframe() != 1;
}

void AppEngine::stopFade() {
    AnimPlayerQuad* anim = m_quads.getAnimPlayer(FADE_ANIM);
    anim->stopAnim();
    anim->snapToFrame(1);
    m_fadeDone = true;
}

void AppEngine::performChangeScene(int scene, int param) {
    if (m_scene != 0) {
        m_scene->end();
        m_scene = 0;
        stopFade();
    }
    m_sceneId = scene;
    if (scene == 3) {
        m_scene = new SceneGame(this);
    } else if (scene == 2) {
        m_scene = new SceneMenu(this);
    } else if (scene == 1) {
        m_scene = new SceneStartup(this);
    }
    m_scene->start(param);
    repaint();
}

void AppEngine::startFadeIn(bool instant) {
    int from = 0;
    if (!instant) {
        from = 2;
    }
    m_quads.playAnimAcross(FADE_ANIM, 2, from, 1);
    m_fadeDone = false;
}

void AppEngine::startFadeOut(bool instant) {
    int to = 0;
    if (!instant) {
        to = 2;
    }
    m_quads.playAnimAcross(FADE_ANIM, 2, 1, to);
    m_fadeDone = false;
}

void AppEngine::startFadeOut(bool instant, int color) {
    setFadeColor(color);
    startFadeOut(instant);
}

void AppEngine::setFadeColor(int color) {
    QuadMesh* mesh = m_quads.getMesh(FADE_MESH);
    mesh->m_vertexBuffer->setDefaultColor(color | 0xff000000);
}

void AppEngine::initAboutString() {
    CFStringRef version = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), kCFBundleVersionKey);
    CFRetain(version);
    midp::ReferenceCountedPointer<midp::String> text(new midp::String(version));
    m_text.dynamicString(-11, 0x802, static_cast<midp::String*>(text));
}

midp::StringBuffer* AppEngine::getStatLabel(int textId) {
    midp::StringBuffer* buffer = m_text.clearStringBuffer();
    m_text.appendStringIdToBuffer(buffer, textId);
    m_text.appendStringIdToBuffer(buffer, 0x80d);
    return buffer;
}

void AppEngine::appendStatValue(midp::StringBuffer* buffer, StatType type, int value) {
    if (type == STAT_COUNT) {
        m_text.appendIntToBuffer(buffer, value);
        return;
    }
    if (type == STAT_OF) {
        if (value >= 0) {
            m_text.appendIntToBuffer(buffer, (value & 0xffffff) >> 16);
            m_text.appendStringIdToBuffer(buffer, 0x80a);
            m_text.appendIntToBuffer(buffer, value & 0xff);
            return;
        }
        buffer->append('-');
        m_text.appendStringIdToBuffer(buffer, 0x80a);
        buffer->append('-');
        return;
    }
    if (type == STAT_TIME_OR_EMPTY && value < 0) {
        m_text.appendStringIdToBuffer(buffer, 0x811);
        return;
    }
    if (type != STAT_TIME && type != STAT_TIME_OR_EMPTY) {
        return;
    }
    m_text.appendMillisTimeToBuffer(buffer, value, 2);
}

void AppEngine::drawStatString(midp::Graphics* g, int font, int textId, StatType type, int value, int x,
                               int y, int anchor) {
    midp::StringBuffer* buffer;
    if (textId == 0x800) {
        buffer = m_text.clearStringBuffer();
        appendStatValue(buffer, type, value);
    } else {
        buffer = getStatLabel(textId);
        buffer->append(' ');
        appendStatValue(buffer, type, value);
    }
    m_text.drawString(g, buffer, font, x, y, anchor);
}

void AppEngine::drawOfStatString(midp::Graphics* g, int font, int textId, int value, int total, int x,
                                 int y, int anchor) {
    drawStatString(g, font, textId, STAT_OF, (value << 16) | total, x, y, anchor);
}

void AppEngine::pointerReleased(int x, int y, int pointerId) {
    addPointerEvent(2, x, y, pointerId, 0);
    m_windows.pointerReleased(x, y, pointerId);
}

void AppEngine::pointerDragged(int x, int y, int pointerId) {
    addPointerEvent(1, x, y, pointerId, 0);
    m_windows.pointerDragged(x, y, pointerId);
}

void AppEngine::pointerPressed(int x, int y, int pointerId, int tapCount) {
    addPointerEvent(0, x, y, pointerId, tapCount);
    m_windows.pointerPressed(x, y, pointerId);
}

void AppEngine::paint(midp::Graphics* g) {
    if (m_running) {
        if (m_scene != 0) {
            m_scene->render(g);
        }
    }
    m_windows.render(g);
#ifdef MEDECOMP_PORT
    medecomp_menu_focus_draw(g);
#endif
    m_quads.render(g, 4);
    if (m_achievementNote != 0) {
        m_achievementNote->render(g, 0, 0);
    }
#ifdef MEDECOMP_PORT
    medecomp_skin_preview(g);
    medecomp_level_preview(g);
#endif
    m_fadeDone = m_quads.isAnimating(FADE_ANIM) == 0;
    m_painting = false;
}

void AppEngine::update(int dt) {
    if (m_achievementNote != 0) {
        m_achievementNote->update(dt);
    }
    m_windows.update(dt);
    if (dt > 0x87) {
        dt = 0x87;
    }
    if (m_nextScene != -1) {
        performChangeScene(m_nextScene, m_nextSceneParam);
        m_nextScene = -1;
        m_nextSceneParam = -1;
    }
    if (m_scene != 0) {
        m_quads.updateAnim(FADE_ANIM, dt);
        m_quads.updateAnim(10, dt);
        processPointerEvents();
        m_scene->update(dt);
    }
}

void AppEngine::runLoop(int dt) {
    update(dt);
}

void AppEngine::resumeGame() {
    m_sound.resume();
    if (m_scene != 0) {
        m_scene->resume();
    }
    m_paused = false;
}

void AppEngine::pauseGame() {
    m_paused = true;
    while (m_painting) {
        midp::Thread::yield();
    }
    if (m_scene != 0) {
        m_scene->pause();
    }
    m_sound.pause();
}

void AppEngine::hideNotify() {
    pauseGame();
}

void AppEngine::stopThread() {
    while (m_updating || m_painting) {
        midp::Thread::yield();
    }
    AppEngineThread* thread = m_thread;
    m_thread = 0;
    if (thread != 0) {
        thread->join();
        midp::DECREF(thread);
    }
}

void AppEngine::end() {
    m_running = false;
    stopThread();
    if (m_scene != 0) {
        m_scene->end();
    }
    saveRMSAppSettings();
}

void AppEngine::startThread() {
    m_running = true;
    if (m_thread == 0) {
        m_thread = new AppEngineThread(this);
        midp::INCREF(m_thread);
        m_thread->start();
    }
}

void AppEngine::showNotify() {
    if (!m_paused) {
        return;
    }
    resumeGame();
    startThread();
}

void AppEngine::doStartupLoading() {
    if (m_startupDone) {
        return;
    }
    m_startupDone = true;
    initAboutString();
    m_achievementNote = new UI::AchievementNotification();
}

void AppEngine::start() {
    loadRMSAppSettings();
    m_text.init();
    changeScene(1);
    m_bgMusic = new BGMusic(m_sound);
    midp::Runnable* music = 0;
    if (m_bgMusic != 0) {
        music = m_bgMusic;
    }
    m_bgThread = new midp::Thread(music);
    m_bgThread->start();
    getM3GAssets()->loadData();
    m_quads.loadQuadData(ResourceManager::loadBinaryFile(0xdc), 1024, 768);
#ifdef MEDECOMP_PORT
    m_quads.setMeshBounds(FADE_MESH, (float)-medecomp_wide_extra(), 0.0f, 1024.0f + 2 * medecomp_wide_extra(), 768.0f, 9);
#endif
    m_quads.loadQuads(1);
    m_paused = false;
    ea::EASpywareManager::getInstance()->appStarted(m_firstLaunch != 0);
}

AppEngine::AppEngine(MonkeyApp* midlet)
    : m_unk14(0),
      m_saveFileError(false),
      m_thread(0),
      m_painting(false),
      m_updating(false),
      m_midlet(midlet),
      m_running(false),
      m_scene(0),
      m_paused(true),
      m_graphics3d(0),
      m_bgMusic(0),
      m_pointerBuffer(0),
      m_unk408(false),
      m_startupDone(false),
      m_pointerCounts(2),
      m_pointerTypes(midp::array2D<int>(2, 0x32)),
      m_pointerX(midp::array2D<int>(2, 0x32)),
      m_pointerY(midp::array2D<int>(2, 0x32)),
      m_pointerIds(midp::array2D<int>(2, 0x32)),
      m_pointerTaps(midp::array2D<int>(2, 0x32)),
      m_fadeDone(false),
      m_sceneId(0),
      m_nextScene(2),
      m_nextSceneParam(0),
      m_unk7e4(0),
      m_unk7e8(0),
      m_stringBuffer(new midp::StringBuffer(15)),
      m_unk7f0(false),
      m_osVersion(0),
      m_firstLaunch(false),
      m_loadingWorld(0),
      m_loadingCamera(0),
      m_loadingTransform(0),
      m_loadingNode(0),
      m_loadingAppearance(0),
      m_loadingTexture(0),
      m_loadingScroll(0.0f),
      m_scrollingLoad(false),
      m_mediaPicker(0),
      m_achievementNote(0),
      m_tutorialBoxes(false),
      m_uploadScores(false),
      m_unk8d6(false),
      m_spywareEnabled(true),
      m_mayhemToken(0),
      m_challengeTime(0),
      m_launchedForChallenge(false),
      m_storageFull(false) {
    AnimationManager::constructAnimationManager(m_animManData);
    AnimationManager::loadSubimageFile(m_animManData, m_resources);
    AnimationManager::loadAnimFile(m_animManData, m_resources);
    AnimationManager::loadColorsFile(m_animManData, m_resources);
    m_graphics3d = m3g::Graphics3D::getInstance();
    loadAnimManager3D();
    m_mediaPicker = new MediaPicker();
    midp::INCREF(m_mediaPicker);
    if (m3g::g_usingGLES20) {
        m_reflection = new ReflectionMapShader();
    }
    m_sound.setListenerOrientation(0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
    loadGameOptions();
    ea::EASpywareManager::getInstance()->setLanguage(m_text.getCurrentLocale());
    ea::EASpywareManager::getInstance()->setEnabled(m_spywareEnabled);
    midp::RuntimeIPhone* runtime = static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime());
    m_launchedForChallenge = runtime->isPushStartup();
    if (m_launchedForChallenge) {
        runtime = static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime());
        void* option = runtime->getPushOption(CFSTR("challengeId"));
        const char* text = CFStringGetCStringPtr(static_cast<CFStringRef>(option), CFStringGetSystemEncoding());
        m_challengeId = text;
    }
    loadPendingChallenges();
    GameCenter gameCenter;
    if (!gameCenter.isGameCenterAPIAvailable()) {
        puts("GameCenter seems to be unavailable!");
    } else {
        puts("GameCenter is ok!");
        gameCenter.authenticateLocalPlayer();
    }
}

AppEngine::~AppEngine() {
    m_scene = 0;
    m_midlet = 0;
    m_bgMusic->beQuiet();
    m_bgMusic->close();
    m_bgThread = 0;
    midp::DECREF(m_mediaPicker);
    m_mediaPicker = 0;
    if (m_achievementNote != 0) {
        delete m_achievementNote;
    }
    freeLoadingScreens();
    if (m_osVersion != 0) {
        midp::DECREF(m_osVersion);
        m_osVersion = 0;
    }
    savePendingChallenges();
}

void AppEngine::setChallengeTime(int timeMs) {
    m_challengeTime = timeMs;
}

int AppEngine::getChallengeTime() const {
    return m_challengeTime;
}

const std::string& AppEngine::getChallengeID() const {
    return m_challengeId;
}

void AppEngine::setChallengeID(const std::string& id) {
    m_challengeId = id;
}

int AppEngine::getPendingChallengeCount() const {
    return static_cast<int>(m_pendingChallenges.size());
}

PendingChallenge AppEngine::getNextPendingChallenge() {
    return m_pendingChallenges.front();
}

void AppEngine::popPendingChallenge() {
    m_pendingChallenges.pop();
}

void AppEngine::clearPendingChallenges() {
    while (m_pendingChallenges.size() != 0) {
        m_pendingChallenges.pop();
    }
}

bool AppEngine::storageFull() const {
    return m_storageFull;
}

bool AppEngine::getTutorialBoxes() const {
    return m_tutorialBoxes;
}

bool AppEngine::launchedForChallenge() {
    return m_launchedForChallenge;
}

void AppEngine::setStorageFull(bool full) {
    m_storageFull = full;
}

void AppEngine::setTutorialBoxes(bool show) {
    m_tutorialBoxes = show;
}

void AppEngine::setUploadScores(bool upload) {
    m_uploadScores = upload;
}

PhotoPicker* AppEngine::getPhotoPicker() {
    return &m_photoPicker;
}
