#include "game/UI/SoundOptions.h"

#include "game/AppEngine.h"
#include "game/QuadManager.h"
#include "game/SoundManager.h"
#include "game/TextManager.h"
#include "runtime/midp/RuntimeIPhone.h"

namespace UI {

static const int TEXT_TITLE = 0x8d7;
static const int TEXT_SUBTITLE = 0x81e;
static const int TEXT_MUSIC = 0x8d8;
static const int TEXT_EFFECTS = 0x8d9;
static const int FONT_LABEL = 7;
static const int LABEL_X = 0xf0;
static const int LABEL_DY = 0x14;
static const int LABEL_ANCHOR = 0x11;
static const int SLIDER_WIDTH = 0x163;
static const int SLIDER_X = 0x172;
static const int OFFSCREEN = -500;
static const int GROUP_SLIDER = 0x35;
static const int MESH_LOW = 0x7e;
static const int MESH_HIGH = 0x7f;
static const int ICON_GAP = 0x18;
static const int ANCHOR_LOW = QuadManager::ANCHOR_VCENTER | QuadManager::ANCHOR_RIGHT;
static const int ANCHOR_HIGH = QuadManager::ANCHOR_VCENTER | QuadManager::ANCHOR_LEFT;
static const int QUAD_SCOPE = 2;

float SoundOptions::getMusicLevel() const {
    return m_music.getValue();
}

float SoundOptions::getEffectsLevel() const {
    return m_effects.getValue();
}

bool SoundOptions::pointerPressed(int x, int y, int id) {
    TitledWindow::pointerPressed(x, y, id);
    if (id > 0) {
        return false;
    }
    HorizontalSliderElement* slider = &m_music;
    if (!slider->contains(x, y)) {
        slider = &m_effects;
        if (!slider->contains(x, y)) {
            return false;
        }
    }
    slider->pointerPressed(slider->toRelativeX(x), y, id);
    return true;
}

bool SoundOptions::pointerDragged(int x, int y, int id) {
    if (id > 0) {
        return false;
    }
    HorizontalSliderElement* slider = &m_music;
    if (!slider->contains(x, y)) {
        slider = &m_effects;
        if (!slider->contains(x, y)) {
            return false;
        }
    }
    slider->pointerDragged(slider->toRelativeX(x), y, id);
    return true;
}

void SoundOptions::update(int dt) {
    TitledWindow::update(dt);
    m_music.update(dt);
    m_effects.update(dt);
}

static void drawSliderIcons(QuadManager& quads, midp::Graphics* g, const WindowElement& slider) {
    float y = (float)(slider.getY() + LABEL_DY);
    quads.setMeshPosition(MESH_LOW, (float)(slider.getX() - ICON_GAP), y, ANCHOR_LOW);
    quads.setMeshPosition(MESH_HIGH, (float)(slider.getX() + slider.getWidth() + ICON_GAP), y, ANCHOR_HIGH);
    quads.render(g, QUAD_SCOPE);
}

void SoundOptions::render(midp::Graphics* g, int originY, int originX) {
    TitledWindow::render(g, originY, originX);
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    text->drawString(g, TEXT_MUSIC, FONT_LABEL, LABEL_X, m_music.getY() + LABEL_DY, LABEL_ANCHOR);
    text->drawString(g, TEXT_EFFECTS, FONT_LABEL, LABEL_X, m_effects.getY() + LABEL_DY, LABEL_ANCHOR);

    QuadManager& quads = *m_quadManager;
    quads.setGroupVisible(GROUP_SLIDER, true);
    quads.setMeshVisible(MESH_LOW, true);
    quads.setMeshVisible(MESH_HIGH, true);
    drawSliderIcons(quads, g, m_music);
    drawSliderIcons(quads, g, m_effects);
    quads.setMeshVisible(MESH_HIGH, false);
    quads.setMeshVisible(MESH_LOW, false);
    quads.setGroupVisible(GROUP_SLIDER, false);

    m_music.render(g, originY, originX);
    m_effects.render(g, originY, originX);
}

bool SoundOptions::pointerReleased(int x, int y, int id) {
    TitledWindow::pointerReleased(x, y, id);
    if (id > 0) {
        return false;
    }
    HorizontalSliderElement* slider = &m_music;
    if (!slider->contains(x, y)) {
        slider = &m_effects;
        if (!slider->contains(x, y)) {
            return false;
        }
    }
    slider->pointerReleased(slider->toRelativeX(x), y, id);
    return true;
}

SoundOptions::SoundOptions()
    : TitledWindow(TEXT_TITLE, TEXT_SUBTITLE), m_music(SLIDER_WIDTH), m_effects(SLIDER_WIDTH) {
    AppEngine* canvas = AppEngine::getCanvas();
    int width = canvas->getWidth();
    int height = canvas->getHeight();
    m_background.setPosition(0x6e, 0xdc);
    m_background.setDimensions(width - 0xdc, height - 0x1b8);

    int center = height >> 1;
    int musicY;
    int effectsY;
    int musicX;
    if (static_cast<midp::RuntimeIPhone*>(midp::Runtime::getRuntime())->isOtherAudioPlaying()) {
        musicX = OFFSCREEN;
        musicY = OFFSCREEN;
        effectsY = center - 0x14;
    } else {
        musicX = SLIDER_X;
        musicY = center - 0x39;
        effectsY = center + 0x11;
    }
    m_music.setPosition(musicX, musicY);
    m_effects.setPosition(SLIDER_X, effectsY);

    SoundManager& sound = canvas->getSoundManager();
    m_music.setValue(sound.getVolumeMusic());
    m_effects.setValue(sound.getVolumeSFX());
}

SoundOptions::~SoundOptions() {}

}
