#include "game/GameUI/SoundSettings.h"

#include "game/AppEngine.h"
#include "game/GameCommon.h"
#include "game/GameObjectPlayer.h"
#include "game/GameScreen.h"
#include "game/SceneGame.h"
#include "game/SoundManager.h"
#include "runtime/midp/Math.h"
#include "runtime/midp/RuntimeIPhone.h"

namespace GameUI {

static const int TEXT_TITLE = 0x8d7;
static const int TEXT_HEADER = 0x81e;
static const int TEXT_MUSIC = 0x8d8;
static const int TEXT_EFFECTS = 0x8d9;
static const int TEXT_BACK = 0x82f;
static const float SLIDER_MAX = 1.0f;
static const int WINDOW_PAD = 150;
static const int EDGE = 20;
static const int SLIDER_GAP = 50;
static const int SOUND_TICK = 35;

static bool samePlayer(int owner, GameScreen* screen) {
    if (owner == -1) {
        return true;
    }
    return screen->getPlayerObject()->getPlayerIndex() == owner;
}

SoundSettings::SoundSettings(SceneGame* scene, GameScreen* screen)
    : m_scene(scene),
      m_screen(screen),
      m_backing(TEXT_TITLE, TEXT_HEADER),
      m_buttons(TEXT_BACK, screen),
      m_music(TEXT_MUSIC, SLIDER_MAX),
      m_effects(TEXT_EFFECTS, SLIDER_MAX) {
    int width = screen->getLocalWidth();
    int height = screen->getLocalHeight();
    int labelW = midp::Math::max(m_music.getLabelWidth(), m_effects.getLabelWidth());
    int sliderH = midp::Math::max(m_music.getHeight(), m_effects.getHeight());
    int windowH = WINDOW_PAD + sliderH * 2;
    int windowW = (width >> 2) * 3;
    int left = (width - windowW) >> 1;
    int top = (height - windowH) >> 1;
    m_backing.setBounds(left, top, windowW, windowH, 0);
    int labelX = left + EDGE;
    int trackLeft = labelX + labelW + EDGE;
    int trackRight = left + windowW - EDGE;
    m_music.setCoordinates(labelX, trackLeft, trackRight);
    m_effects.setCoordinates(labelX, trackLeft, trackRight);

    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    m_music.setValue(sound.getVolumeMusic());
    m_effects.setValue(sound.getVolumeSFX());
}

bool SoundSettings::canModifyMusicSlider() {
    return samePlayer(m_scene->getSoundMusicModifying(), m_screen);
}

bool SoundSettings::canModifyEffectSlider() {
    return samePlayer(m_scene->getSoundEffectModifying(), m_screen);
}

int SoundSettings::pointerReleased(int pressX, int pressY, int x, int y, int id) {
    if (m_music.isPressed()) {
        m_music.pointerReleased(id);
        m_scene->clearSoundMusicModifying();
    }
    if (m_effects.isPressed()) {
        m_effects.pointerReleased(id);
        m_scene->clearSoundEffectModifying();
    }
    return m_buttons.pointerReleased(pressX, pressY, x, y) == BottomButtonSet::RESULT_BUTTON ? 1 : 0;
}

void SoundSettings::pointerDragged(int pressX, int pressY, int x, int y, int id) {
    int music = m_music.pointerDragged(x, y, id);
    int effects = m_effects.pointerDragged(x, y, id);
    if (music != 0 || effects != 0) {
        SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
        if (music != 0) {
            sound.setVolumeMusic(m_music.getValue());
        }
        if (effects != 0) {
            sound.setVolumeSFX(m_effects.getValue());
            sound.playEvent(SOUND_TICK, 1.0f);
        }
    }
    m_buttons.pointerDragged(pressX, pressY, x, y);
}

void SoundSettings::pointerPressed(int x, int y, int id) {
    midp::RuntimeIPhone* runtime = static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime());
    if (canModifyMusicSlider() && !runtime->isOtherAudioPlaying()) {
        m_music.pointerPressed(x, y, id);
        if (m_music.isPressed()) {
            m_scene->setSoundMusicModifying(m_screen->getPlayerObject()->getPlayerIndex());
        }
    }
    if (canModifyEffectSlider()) {
        m_effects.pointerPressed(x, y, id);
        if (m_effects.isPressed()) {
            m_scene->setSoundEffectModifying(m_screen->getPlayerObject()->getPlayerIndex());
        }
    }
    m_buttons.pointerPressed(x, y);
}

void SoundSettings::render2D(midp::Graphics* g) {
    m_backing.render2D(g);
    m_buttons.render2D(g);
    if (!static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOtherAudioPlaying()) {
        m_music.render2D(g);
    }
    m_effects.render2D(g);
}

void SoundSettings::render3D(m3g::Graphics3D* g3d) {
    SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
    if (!canModifyMusicSlider()) {
        m_music.setValue(sound.getVolumeMusic());
    }
    if (!canModifyEffectSlider()) {
        m_effects.setValue(sound.getVolumeSFX());
    }
    m_backing.render3D(g3d);
    m_buttons.render3D(g3d);
    int center = m_backing.getTop() + (m_backing.getHeight() >> 1);
    if (static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOtherAudioPlaying()) {
        m_effects.setY((float)center);
        m_effects.render3D(g3d, canModifyEffectSlider());
        return;
    }
    int musicY = center - ((m_music.getHeight() + SLIDER_GAP) >> 1);
    int effectsY = center + ((m_effects.getHeight() + SLIDER_GAP) >> 1);
    m_music.setY((float)musicY);
    m_effects.setY((float)effectsY);
    m_music.render3D(g3d, canModifyMusicSlider());
    m_effects.render3D(g3d, canModifyEffectSlider());
}

SoundSettings::~SoundSettings() {}

}
