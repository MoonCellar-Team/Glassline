#include "port/platform.h"

#include <math.h>
#include <pthread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <vector>

#include "game/AppEngine.h"
#include "game/GameScreen.h"
#include "game/MenuMain.h"
#include "game/MenuMainSubMenu.h"
#include "game/MenuPause.h"
#include "game/QuadManager.h"
#include "game/SceneGame.h"
#include "game/SceneMenu.h"
#include "game/SoundManager.h"
#include "game/GameUI/Confirmation.h"
#include "game/GameUI/EndOfLevelResults.h"
#include "game/GameUI/ScrollingText.h"
#include "game/GameUI/SoundSettings.h"
#include "game/UI/AboutMenu.h"
#include "game/UI/AboutWindow.h"
#include "game/UI/AchievementWindow.h"
#include "game/UI/ChallengeCommentEntry.h"
#include "game/UI/ChallengeEndOfLevelPrompt.h"
#include "game/UI/ChallengeUserList.h"
#include "game/UI/ChallengeUserWindow.h"
#include "game/UI/ChapterSelectMultiplayerWindow.h"
#include "game/UI/ChapterSelectWindow.h"
#include "game/UI/CheckBox.h"
#include "game/UI/ConfirmationWindow.h"
#include "game/UI/DisplayNameEntry.h"
#include "game/UI/EndOfGameResultsWindow.h"
#include "game/UI/EndOfLevelPrompt.h"
#include "game/UI/GameOptions.h"
#include "game/UI/HelpWindow.h"
#include "game/UI/LanguageWindow.h"
#include "game/UI/LeaderboardWindow.h"
#include "game/UI/NotchedSlider.h"
#include "game/UI/PendingChallengeWindow.h"
#include "game/UI/PleaseWaitWindow.h"
#include "game/UI/SliderElement.h"
#include "game/UI/SoundOptions.h"
#include "game/UI/SpeedRunEndOfLevelPrompt.h"
#include "game/UI/TitledWindow.h"
#include "game/UI/UnlockableWindow.h"
#include "game/UI/UploadConfirmationWindow.h"
#include "game/UI/WindowStore.h"
#include "levels.h"
#include "runtime/midp/Graphics.h"

extern "C" {
extern const char _ZTVN2UI6ButtonE[];
extern const char _ZTVN2UI11MajorButtonE[];
extern const char _ZTVN4port8platform8LevelRowE[];
extern const char _ZTVN2UI15LeftArrowButtonE[];
extern const char _ZTVN2UI16RightArrowButtonE[];
extern const char _ZTVN2UI8CheckBoxE[];
extern const char _ZTVN2UI13SliderElementE[];
extern const char _ZTVN2UI23HorizontalSliderElementE[];
extern const char _ZTVN2UI13NotchedSliderE[];
extern const char _ZTVN2UI22ChapterArrowedScrollerE[];
extern const char _ZTVN2UI15UnlockablePanelE[];
extern const char _ZTVN2UI13LanguagePanelE[];
extern const char _ZTVN2UI14AboutMenuPanelE[];
extern const char _ZTVN2UI12TitledWindowE[];
extern const char _ZTVN2UI12SoundOptionsE[];
extern const char _ZTVN2UI9AboutMenuE[];
extern const char _ZTVN2UI14LanguageWindowE[];
extern const char _ZTVN2UI17AchievementWindowE[];
extern const char _ZTVN2UI19ChapterSelectWindowE[];
extern const char _ZTVN2UI24ChapterSelectStoryWindowE[];
extern const char _ZTVN2UI27ChapterSelectSpeedrunWindowE[];
extern const char _ZTVN2UI30ChapterSelectMultiplayerWindowE[];
extern const char _ZTVN2UI11GameOptionsE[];
extern const char _ZTVN2UI11AboutWindowE[];
extern const char _ZTVN2UI10HelpWindowE[];
extern const char _ZTVN2UI16UnlockableWindowE[];
extern const char _ZTVN4port8platform18PlayerLevelsWindowE[];
extern const char _ZTVN2UI18ConfirmationWindowE[];
extern const char _ZTVN2UI22EnableShareDataConfirmE[];
extern const char _ZTVN2UI20ExitToBrowserConfirmE[];
extern const char _ZTVN2UI9NoStorageE[];
extern const char _ZTVN2UI23DisableShareDataConfirmE[];
extern const char _ZTVN2UI14NewGameConfirmE[];
extern const char _ZTVN2UI15TutorialConfirmE[];
extern const char _ZTVN2UI16ChallengeConfirmE[];
extern const char _ZTVN2UI24ShareDataDisabledConfirmE[];
extern const char _ZTVN2UI23ShareDataEnabledConfirmE[];
extern const char _ZTVN2UI16DisplayNameEntryE[];
extern const char _ZTVN2UI16EndOfLevelPromptE[];
extern const char _ZTVN2UI25ChallengeEndOfLevelPromptE[];
extern const char _ZTVN2UI24SpeedRunEndOfLevelPromptE[];
extern const char _ZTVN2UI21StoryEndOfLevelPromptE[];
extern const char _ZTVN2UI22EndOfGameResultsWindowE[];
extern const char _ZTVN2UI17LeaderboardWindowE[];
extern const char _ZTVN2UI16PleaseWaitWindowE[];
extern const char _ZTVN2UI19ChallengeUserWindowE[];
extern const char _ZTVN2UI21ChallengeCommentEntryE[];
extern const char _ZTVN2UI24UploadConfirmationWindowE[];
extern const char _ZTVN2UI22PendingChallengeWindowE[];
extern const char _ZTV8MenuMain[];
}

namespace port {
namespace platform {

namespace {

enum {
    CmdLeft = MenuFocusLeft,
    CmdRight = MenuFocusRight,
    CmdUp = MenuFocusUp,
    CmdDown = MenuFocusDown,
    CmdActivate = MenuFocusActivate,
    CmdBack = MenuFocusBack
};

const int kBackId = 0x82f;

enum Kind { KButton, KCheck, KSlider, KNotch, KGame, KHelp, KSound, KPane, KUser };

const int kUserRow = 0x38;
const int kTitleBottom = -7;

struct Target {
    int x = 0, y = 0, w = 0, h = 0;
    Kind kind = KButton;
    void* ptr = nullptr;
    void* owner = nullptr;
    bool back = false;
    int extra = 0;
    int fx = 0, fy = 0, fw = 0, fh = 0;
    int ox = 0, oy = 0;
};

std::atomic<bool> s_keyboard{false};
bool s_frameOn = false;
bool s_frameTint = false;
int s_frame[4];

std::atomic<int> s_cmd{0};
std::atomic<int> s_count{0};
std::atomic<int> s_scroll{0};
std::atomic<int> s_steps{0};
void* s_focus = nullptr;
int s_userRow = -1;
UI::Button* s_uiLit = nullptr;
GameUI::Button* s_gameLit = nullptr;
MenuMainSubMenu* s_arm = nullptr;

void light(UI::Button* ui, GameUI::Button* game) {
    if (s_uiLit && s_uiLit != ui) s_uiLit->unpress();
    if (s_gameLit && s_gameLit != game) s_gameLit->m_pressed = false;
    s_uiLit = ui;
    s_gameLit = game;
    if (ui && ui->m_enabled) ui->m_pressed = true;
    if (game) game->m_pressed = true;
}

void clearLight() { light(nullptr, nullptr); }

bool alive(int x, int y, int w, int h) { return w > 0 && h > 0 && x > -200 && y > -200 && x + w > 0 && y + h > 0; }

void add(std::vector<Target>& out, Target t) {
    if (!alive(t.x, t.y, t.w, t.h) || !t.ptr) return;
    for (const Target& o : out)
        if (o.ptr == t.ptr) return;
    out.push_back(t);
}

void addUi(std::vector<Target>& out, UI::Button* b, UI::Window* w) {
    if (!b || !b->m_enabled) return;
    Target t;
    t.x = b->getX();
    t.y = b->getY();
    t.w = b->getWidth();
    t.h = b->getHeight();
    t.kind = KButton;
    t.ptr = b;
    t.owner = w;
    t.back = b->getStringId() == kBackId;
    add(out, t);
}

void addCheck(std::vector<Target>& out, UI::CheckBox* c, UI::Window* w) {
    if (!c) return;
    Target t;
    t.x = c->getX();
    t.y = c->getY();
    t.w = c->getWidth();
    t.h = c->getHeight();
    t.kind = KCheck;
    t.ptr = c;
    t.owner = w;
    add(out, t);
}

void addSlider(std::vector<Target>& out, UI::SliderElement* s, UI::Window* w) {
    if (!s) return;
    Target t;
    t.x = s->getX();
    t.y = s->getY();
    t.w = s->getWidth();
    t.h = s->getHeight();
    t.kind = KSlider;
    t.ptr = s;
    t.owner = w;
    add(out, t);
}

void addNotch(std::vector<Target>& out, UI::NotchedSlider* n, UI::Window* w) {
    if (!n) return;
    Target t;
    t.x = n->getX();
    t.y = n->getY();
    t.w = n->getWidth();
    t.h = n->getHeight();
    t.kind = KNotch;
    t.ptr = n;
    t.owner = w;
    add(out, t);
}

void addRibbon(std::vector<Target>& out, UI::NotchedSlider* n, UI::Button* left, UI::Button* right, UI::Window* w) {
    size_t before = out.size();
    addNotch(out, n, w);
    if (out.size() == before) return;
    Target& t = out.back();
    int x0 = t.x, y0 = t.y, x1 = t.x + t.w, y1 = t.y + t.h;
    for (UI::Button* b : {left, right})
        if (b && alive(b->getX(), b->getY(), b->getWidth(), b->getHeight())) {
            x0 = std::min(x0, b->getX());
            y0 = std::min(y0, b->getY());
            x1 = std::max(x1, b->getX() + b->getWidth());
            y1 = std::max(y1, b->getY() + b->getHeight());
        }
    t.fx = x0, t.fy = y0, t.fw = x1 - x0, t.fh = y1 - y0;
}

void addGame(std::vector<Target>& out, GameUI::Button* b, GameScreen* screen, bool back) {
    if (!b || b->getLabelId() == GameUI::Button::LABEL_NONE) return;
    Target t;
    t.x = b->m_x;
    t.y = b->m_y;
    t.w = b->getWidth();
    t.h = b->getHeight();
    t.kind = KGame;
    t.ptr = b;
    t.owner = screen;
    t.back = back || b->getLabelId() == kBackId;
    add(out, t);
}

bool sameV(const void* obj, const char* vt) {
    const void* want = vt + sizeof(void*) * 2;
    return obj && *reinterpret_cast<const void* const*>(obj) == want;
}

template <class T>
T* one(void* obj, const char* vt) {
    return sameV(obj, vt) ? static_cast<T*>(obj) : nullptr;
}

template <class T>
T* any(void* obj, const char* const* vts, int n) {
    for (int i = 0; i < n; i++)
        if (sameV(obj, vts[i])) return static_cast<T*>(obj);
    return nullptr;
}

bool isButton(const void* el) {
    return sameV(el, _ZTVN2UI6ButtonE) || sameV(el, _ZTVN2UI11MajorButtonE) ||
           sameV(el, _ZTVN2UI15LeftArrowButtonE) || sameV(el, _ZTVN2UI16RightArrowButtonE) ||
           sameV(el, _ZTVN4port8platform8LevelRowE);
}
bool isCheck(const void* el) { return sameV(el, _ZTVN2UI8CheckBoxE); }
bool isSlider(const void* el) {
    return sameV(el, _ZTVN2UI13SliderElementE) || sameV(el, _ZTVN2UI23HorizontalSliderElementE);
}
bool isNotch(const void* el) {
    return sameV(el, _ZTVN2UI13NotchedSliderE) || sameV(el, _ZTVN2UI22ChapterArrowedScrollerE) ||
           sameV(el, _ZTVN2UI15UnlockablePanelE) || sameV(el, _ZTVN2UI13LanguagePanelE) ||
           sameV(el, _ZTVN2UI14AboutMenuPanelE);
}

UI::TitledWindow* asTitled(UI::Window* w) {
    const char* vts[] = {
        _ZTVN2UI12TitledWindowE, _ZTVN2UI12SoundOptionsE, _ZTVN2UI9AboutMenuE, _ZTVN2UI14LanguageWindowE,
        _ZTVN2UI17AchievementWindowE, _ZTVN2UI19ChapterSelectWindowE, _ZTVN2UI24ChapterSelectStoryWindowE,
        _ZTVN2UI27ChapterSelectSpeedrunWindowE, _ZTVN2UI30ChapterSelectMultiplayerWindowE, _ZTVN2UI11GameOptionsE,
        _ZTVN2UI11AboutWindowE, _ZTVN2UI10HelpWindowE, _ZTVN2UI16UnlockableWindowE,
        _ZTVN4port8platform18PlayerLevelsWindowE,
    };
    return any<UI::TitledWindow>(w, vts, int(sizeof vts / sizeof vts[0]));
}

UI::ConfirmationWindow* asConfirm(UI::Window* w) {
    const char* vts[] = {
        _ZTVN2UI18ConfirmationWindowE, _ZTVN2UI22EnableShareDataConfirmE, _ZTVN2UI20ExitToBrowserConfirmE,
        _ZTVN2UI9NoStorageE, _ZTVN2UI23DisableShareDataConfirmE, _ZTVN2UI14NewGameConfirmE,
        _ZTVN2UI15TutorialConfirmE, _ZTVN2UI16ChallengeConfirmE, _ZTVN2UI24ShareDataDisabledConfirmE,
        _ZTVN2UI23ShareDataEnabledConfirmE,
    };
    return any<UI::ConfirmationWindow>(w, vts, int(sizeof vts / sizeof vts[0]));
}

UI::EndOfLevelPrompt* asEndLevel(UI::Window* w) {
    const char* vts[] = {_ZTVN2UI16EndOfLevelPromptE, _ZTVN2UI25ChallengeEndOfLevelPromptE,
                         _ZTVN2UI24SpeedRunEndOfLevelPromptE, _ZTVN2UI21StoryEndOfLevelPromptE};
    return any<UI::EndOfLevelPrompt>(w, vts, int(sizeof vts / sizeof vts[0]));
}

UI::ChapterSelectWindow* asChapters(UI::Window* w) {
    const char* vts[] = {_ZTVN2UI19ChapterSelectWindowE, _ZTVN2UI24ChapterSelectStoryWindowE,
                         _ZTVN2UI27ChapterSelectSpeedrunWindowE};
    return any<UI::ChapterSelectWindow>(w, vts, int(sizeof vts / sizeof vts[0]));
}

UI::Window* paneOf(UI::Window* w) {
    if (auto* h = one<UI::HelpWindow>(w, _ZTVN2UI10HelpWindowE)) return h->m_panel.get();
    if (auto* a = one<UI::AboutWindow>(w, _ZTVN2UI11AboutWindowE)) return a->m_panel.get();
    if (auto* a = one<UI::AchievementWindow>(w, _ZTVN2UI17AchievementWindowE)) return &a->m_list;
    if (auto* l = one<UI::LeaderboardWindow>(w, _ZTVN2UI17LeaderboardWindowE)) return &l->m_list;
    if (auto* c = one<UI::ChallengeUserWindow>(w, _ZTVN2UI19ChallengeUserWindowE)) return &c->m_users;
    return nullptr;
}

bool scrollPane(UI::Window* p, int px) {
    int before = p->getClientOffsetY();
    p->setClientOffsetY(before - px);
    return p->getClientOffsetY() != before;
}

int paneStep(UI::Window* p) { return std::max(48, p->getClientHeight() / 4); }

UI::ChallengeUserList* userRows(UI::Window* w) {
    auto* c = one<UI::ChallengeUserWindow>(w, _ZTVN2UI19ChallengeUserWindowE);
    UI::ChallengeUserList* l = c ? c->m_users.m_list : nullptr;
    return l && !l->m_userIds.empty() ? l : nullptr;
}

void addUsers(std::vector<Target>& out, UI::ChallengeUserWindow* c) {
    UI::ChallengeUserList* l = userRows(c);
    if (!l) return;
    UI::Window& p = c->m_users;
    for (size_t i = 0; i < l->m_userIds.size(); i++) {
        Target t;
        t.x = p.m_clientX + p.getClientOffsetX() + l->getX();
        t.y = p.m_clientY + p.getClientOffsetY() + l->getY() + int(i) * kUserRow;
        t.w = l->getWidth();
        t.h = kUserRow;
        t.kind = KUser;
        t.ptr = &l->m_userIds[i];
        t.owner = c;
        t.extra = int(i);
        add(out, t);
    }
}

void showUserRow(UI::Window* w, bool moved) {
    UI::ChallengeUserList* l = userRows(w);
    s_userRow = -1;
    for (size_t i = 0; l && i < l->m_userIds.size(); i++)
        if (s_focus == &l->m_userIds[i]) s_userRow = int(i);
    if (s_userRow < 0 || !moved) return;
    UI::Window& p = static_cast<UI::ChallengeUserWindow*>(w)->m_users;
    int top = l->getY() + s_userRow * kUserRow;
    if (top + p.getClientOffsetY() < 0) p.setClientOffsetY(-top);
    else if (top + kUserRow + p.getClientOffsetY() > p.getClientHeight())
        p.setClientOffsetY(p.getClientHeight() - top - kUserRow);
}

void collectWindow(UI::Window* w, std::vector<Target>& out) {
    UI::Window* p = userRows(w) ? nullptr : paneOf(w);
    if (p) {
        Target t;
        t.x = p->getX();
        t.y = p->getY();
        t.w = p->getWidth();
        t.h = p->getHeight();
        t.kind = KPane;
        t.ptr = p;
        t.owner = w;
        add(out, t);
    }
    for (UI::WindowElement* el : w->m_elements) {
        size_t before = out.size();
        if (isButton(el)) addUi(out, static_cast<UI::Button*>(el), w);
        else if (isCheck(el)) addCheck(out, static_cast<UI::CheckBox*>(el), w);
        else if (isSlider(el)) addSlider(out, static_cast<UI::SliderElement*>(el), w);
        else if (isNotch(el)) addNotch(out, static_cast<UI::NotchedSlider*>(el), w);
        if (out.size() > before) {
            out.back().ox = w->m_clientX + w->m_clientOffsetX;
            out.back().oy = w->m_clientY + w->m_clientOffsetY;
        }
    }
    if (auto* t = asTitled(w)) addUi(out, &t->m_backButton, w);
    if (auto* c = asConfirm(w)) {
        addUi(out, &c->m_leftButton, w);
        addUi(out, &c->m_rightButton, w);
        if (c->m_rightButton.getStringId() >= 0) {
            for (Target& t : out)
                if (t.ptr == &c->m_rightButton) t.back = true;
        }
    }
    if (auto* d = one<UI::DisplayNameEntry>(w, _ZTVN2UI16DisplayNameEntryE)) {
        addUi(out, &d->m_backButton, w);
        addUi(out, &d->m_okButton, w);
    }
    if (auto* c = asChapters(w)) {
        addUi(out, &c->m_startButton, w);
        addRibbon(out, &c->m_chapters, &c->m_prevButton, &c->m_nextButton, w);
    }
    if (auto* m = one<UI::ChapterSelectMultiplayerWindow>(w, _ZTVN2UI30ChapterSelectMultiplayerWindowE)) {
        addUi(out, &m->m_startButton, w);
        bool race = m->isRaceMode();
        UI::ChapterArrowedScroller* chapters = race ? &m->m_raceScroller : &m->m_timedScroller;
        UI::ChapterArrowedScroller* mode = &m->m_modeScroller;
        if (!port::platform::soloBagsChoosing()) addRibbon(out, mode, &mode->m_left, &mode->m_right, w);
        addRibbon(out, chapters, &chapters->m_left, &chapters->m_right, w);
        if (!race) {
            addCheck(out, &m->m_time2Min, w);
            addCheck(out, &m->m_time3Min, w);
            addCheck(out, &m->m_time5Min, w);
        }
        if (!port::platform::soloBagsChoosing()) addCheck(out, &m->m_combat, w);
    }
    if (auto* e = asEndLevel(w)) {
        addUi(out, &e->m_restart, w);
        addUi(out, &e->m_continue, w);
        addUi(out, &e->m_next, w);
    }
    if (auto* e = one<UI::ChallengeEndOfLevelPrompt>(w, _ZTVN2UI25ChallengeEndOfLevelPromptE))
        addUi(out, &e->m_challenge, w);
    if (auto* e = one<UI::SpeedRunEndOfLevelPrompt>(w, _ZTVN2UI24SpeedRunEndOfLevelPromptE))
        addUi(out, &e->m_challenge, w);
    if (auto* e = one<UI::EndOfGameResultsWindow>(w, _ZTVN2UI22EndOfGameResultsWindowE)) addUi(out, &e->m_button, w);
    if (auto* l = one<UI::LeaderboardWindow>(w, _ZTVN2UI17LeaderboardWindowE)) {
        addRibbon(out, &l->m_slider, &l->m_left, &l->m_right, w);
        addUi(out, &l->m_back, w);
        addUi(out, &l->m_friendsButton, w);
        addUi(out, &l->m_globalButton, w);
    }
    if (auto* p = one<UI::PleaseWaitWindow>(w, _ZTVN2UI16PleaseWaitWindowE)) {
        addUi(out, &p->m_okButton, w);
        addUi(out, &p->m_retryButton, w);
        addUi(out, &p->m_backButton, w);
    }
    if (auto* c = one<UI::ChallengeUserWindow>(w, _ZTVN2UI19ChallengeUserWindowE)) {
        addUi(out, &c->m_backButton, w);
        addUsers(out, c);
    }
    if (auto* c = one<UI::ChallengeCommentEntry>(w, _ZTVN2UI21ChallengeCommentEntryE)) {
        addUi(out, &c->m_backButton, w);
        addUi(out, &c->m_okButton, w);
    }
    if (auto* u = one<UI::UploadConfirmationWindow>(w, _ZTVN2UI24UploadConfirmationWindowE)) {
        addUi(out, &u->m_noButton, w);
        addUi(out, &u->m_yesButton, w);
        for (Target& t : out)
            if (t.ptr == &u->m_noButton) t.back = true;
    }
    if (auto* p = one<UI::PendingChallengeWindow>(w, _ZTVN2UI22PendingChallengeWindowE)) {
        addUi(out, &p->m_accept, w);
        addUi(out, &p->m_decline, w);
        addUi(out, &p->m_later, w);
        for (Target& t : out)
            if (t.ptr == &p->m_decline) t.back = true;
    }
    if (auto* u = one<UI::UnlockableWindow>(w, _ZTVN2UI16UnlockableWindowE)) {
        if (u->m_phase == 2) addUi(out, &u->m_save, w);
        if (u->m_phase == 0) addRibbon(out, &u->m_panel, &u->m_left, &u->m_right, w);
    }
    if (auto* a = one<UI::AboutMenu>(w, _ZTVN2UI9AboutMenuE)) {
        addRibbon(out, &a->m_panel, &a->m_prevButton, &a->m_nextButton, w);
        addCheck(out, &a->m_share, w);
    }
    if (auto* l = one<UI::LanguageWindow>(w, _ZTVN2UI14LanguageWindowE)) {
        addRibbon(out, &l->m_languages, &l->m_left, &l->m_right, w);
    }
    if (auto* o = one<UI::SoundOptions>(w, _ZTVN2UI12SoundOptionsE)) {
        addSlider(out, &o->m_music, w);
        addSlider(out, &o->m_effects, w);
    }
    if (auto* g = one<UI::GameOptions>(w, _ZTVN2UI11GameOptionsE)) {
        addCheck(out, &g->m_tutorial, w);
    }
}

struct Box {
    int x0, y0, x1, y1;
};

const int kFrameGap = 10;
const int kFrameMinGap = 5;
const int kButtonShadowX = 4, kButtonShadowY = 7;
const int kFrameSpace = 3;

Box boxOf(const Target& t) {
    Box b = t.fw ? Box{t.fx, t.fy, t.fx + t.fw, t.fy + t.fh} : Box{t.x, t.y, t.x + t.w, t.y + t.h};
    b.x0 += t.ox, b.x1 += t.ox, b.y0 += t.oy, b.y1 += t.oy;
    if (t.kind == KSlider || t.kind == KSound) {
        b.x0 -= (b.y1 - b.y0) / 2;
        b.x1 += (b.y1 - b.y0) / 2;
    }
    if ((t.kind == KButton && sameV(t.ptr, _ZTVN2UI11MajorButtonE)) || t.kind == KGame) {
        b.x1 -= kButtonShadowX;
        b.y1 -= kButtonShadowY;
    }
    return b;
}

bool overlaps(const Box& a, const Box& b) { return a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1; }

void avoid(Box& f, const Box& t, const Box& o) {
    if (!overlaps(f, o) || overlaps(t, o)) return;
    int best = -1, cost = 1 << 30;
    int want[4] = {o.y1 + kFrameSpace, o.y0 - kFrameSpace, o.x1 + kFrameSpace, o.x0 - kFrameSpace};
    bool side[4] = {o.y1 <= t.y0, o.y0 >= t.y1, o.x1 <= t.x0, o.x0 >= t.x1};
    int now[4] = {f.y0, f.y1, f.x0, f.x1};
    for (int i = 0; i < 4; i++) {
        if (!side[i]) continue;
        int c = i % 2 == 0 ? want[i] - now[i] : now[i] - want[i];
        if (c > 0 && c < cost) best = i, cost = c;
    }
    if (best == 0) f.y0 = std::min(want[0], t.y0 - kFrameMinGap);
    if (best == 1) f.y1 = std::max(want[1], t.y1 + kFrameMinGap);
    if (best == 2) f.x0 = std::min(want[2], t.x0 - kFrameMinGap);
    if (best == 3) f.x1 = std::max(want[3], t.x1 + kFrameMinGap);
}

void placeFrame(const std::vector<Target>& ts, int index) {
    const Target& cur = ts[index];
    Box t = boxOf(cur);
    Box f{t.x0 - kFrameGap, t.y0 - kFrameGap, t.x1 + kFrameGap, t.y1 + kFrameGap};
    for (int i = 0; i < int(ts.size()); i++)
        if (i != index) avoid(f, t, boxOf(ts[i]));
    if (UI::TitledWindow* w = asTitled(static_cast<UI::Window*>(cur.owner))) {
        int title = w->m_background.getY() + kTitleBottom + kFrameSpace;
        if ((t.y0 + t.y1) / 2 > title && f.y0 < title) f.y0 = title;
    }
    if (cur.owner && sameV(cur.owner, _ZTVN2UI17LeaderboardWindowE)) {
        auto* l = static_cast<UI::LeaderboardWindow*>(cur.owner);
        if (cur.ptr == &l->m_slider) {
            f.x0 = l->m_left.getX() - kFrameGap;
            f.x1 = l->m_right.getX() + l->m_right.getWidth() + kFrameGap;
            f.y0 = std::min(l->m_left.getY(), l->m_right.getY()) - kFrameMinGap;
            f.y1 = l->m_frame.getY() - kFrameSpace;
        }
    }
    s_frameOn = true;
    s_frame[0] = f.x0;
    s_frame[1] = f.y0;
    s_frame[2] = f.x1 - f.x0;
    s_frame[3] = f.y1 - f.y0;
    s_frameTint = cur.kind != KButton && cur.kind != KGame;
}

void activate(const Target& t) {
    int cx = t.x + t.w / 2, cy = t.y + t.h / 2;
    if (t.kind == KUser) {
        auto* c = static_cast<UI::ChallengeUserWindow*>(t.owner);
        if (c->m_users.m_list->pointerReleased(0, t.extra * kUserRow + kUserRow / 2, 0)) c->close(UI::RESULT_OK);
        return;
    }
    if (t.kind == KButton || t.kind == KCheck || t.kind == KNotch) {
        auto* w = static_cast<UI::Window*>(t.owner);
        w->pointerPressed(cx, cy, 0);
        w->pointerReleased(cx, cy, 0);
        return;
    }
    if (t.kind != KGame && t.kind != KHelp) return;
    auto* screen = static_cast<GameScreen*>(t.owner);
    if (t.kind == KHelp || screen->getState() == GameScreen::SCREEN_HELP_MENU) {
        if (screen->m_helpText &&
            screen->m_helpText->pointerReleased(cx, cy, cx, cy, 0, screen) == 1)
            screen->stateTransition(GameScreen::SCREEN_MENU_MAIN);
        return;
    }
    if (screen->getState() == GameScreen::SCREEN_MULTIPLAYER_COMPLETE && screen->m_results) {
        if (screen->m_results->pointerReleased(cx, cy, cx, cy, screen) == 1) {
            screen->m_scene->setExitState(SceneGame::STATE_EXIT_LEVEL);
            screen->m_scene->stateTransition(SceneGame::STATE_EXIT_FADE);
        }
        return;
    }
    if (screen->getState() == GameScreen::SCREEN_SOUND_SETTINGS && screen->m_soundSettings) {
        if (screen->m_soundSettings->pointerReleased(cx, cy, cx, cy, 0) == 1)
            screen->stateTransition(GameScreen::SCREEN_MENU_MAIN);
        return;
    }
    if (screen->m_confirmation) {
        int r = screen->m_confirmation->pointerReleased(cx, cy, cx, cy, screen);
        if (r == GameUI::Confirmation::RESULT_YES) screen->confirmResult(true);
        else if (r == GameUI::Confirmation::RESULT_NO) screen->confirmResult(false);
    }
}

bool nudge(const Target& t, int cmd) {
    int dir = cmd == CmdRight ? 1 : cmd == CmdLeft ? -1 : 0;
    if (!dir) return false;
    if (t.kind == KNotch) {
        auto* n = static_cast<UI::NotchedSlider*>(t.ptr);
        if (dir < 0 && n->canPrev()) {
            n->prev();
            return true;
        }
        if (dir > 0 && n->canNext()) {
            n->next();
            return true;
        }
        return false;
    }
    if (t.kind == KSlider) {
        auto* s = static_cast<UI::SliderElement*>(t.ptr);
        float v = s->getValue() + dir * 0.05f;
        if (v < 0) v = 0;
        if (v > 1) v = 1;
        s->setValue(v);
        return true;
    }
    if (t.kind == KSound) {
        auto* s = static_cast<GameUI::SoundSlider*>(t.ptr);
        float v = s->getValue() + dir * 0.05f * s->m_max;
        if (v < 0) v = 0;
        if (v > s->m_max) v = s->m_max;
        s->setValue(v);
        SoundManager& sound = AppEngine::getCanvas()->getSoundManager();
        if (t.extra == 0) sound.setVolumeMusic(s->getValue());
        else {
            sound.setVolumeSFX(s->getValue());
            sound.playEvent(35, 1.0f);
        }
        return true;
    }
    if (t.kind == KCheck) {
        bool on = static_cast<UI::CheckBox*>(t.ptr)->getChecked();
        if (on != (dir > 0)) activate(t);
        return true;
    }
    return false;
}

bool scrollText(GameUI::ScrollingText* h, int px) {
    int before = h->m_scroll;
    h->m_dragScroll = h->m_scroll;
    h->moveScroller(px);
    h->m_dragScroll = h->m_scroll;
    h->m_prevScroll = h->m_scroll;
    return h->m_scroll != before;
}

bool scrollHelp(const Target& t, int cmd) {
    auto* h = static_cast<GameUI::ScrollingText*>(t.ptr);
    return scrollText(h, (cmd == CmdUp ? -1 : 1) * std::max(48, h->m_viewHeight / 4));
}

void scrollWindow(UI::Window* w, const std::vector<Target>& ts, int px, int steps) {
    if (UI::Window* p = paneOf(w)) {
        scrollPane(p, px);
        return;
    }
    const Target* notch = nullptr;
    for (const Target& t : ts)
        if (t.kind == KNotch && (!notch || t.ptr == s_focus)) notch = &t;
    for (; notch && steps > 0; steps--) nudge(*notch, CmdRight);
    for (; notch && steps < 0; steps++) nudge(*notch, CmdLeft);
}

float midX(const Target& t) { return t.x + t.w * 0.5f; }
float midY(const Target& t) { return t.y + t.h * 0.5f; }

bool sameRow(const Target& a, const Target& b) { return fabsf(midY(a) - midY(b)) <= std::max(a.h, b.h) * 0.5f + 4; }

int nearest(const std::vector<Target>& ts, int from, int cmd) {
    const Target& cur = ts[from];
    int best = -1;
    if (cmd == CmdLeft || cmd == CmdRight) {
        float bestDx = 1.0e9f;
        for (int i = 0; i < int(ts.size()); i++) {
            float dx = (midX(ts[i]) - midX(cur)) * (cmd == CmdRight ? 1 : -1);
            if (i == from || dx <= 4 || !sameRow(cur, ts[i]) || dx >= bestDx) continue;
            bestDx = dx;
            best = i;
        }
        if (best < 0 && sameV(cur.ptr, _ZTVN4port8platform8LevelRowE)) {
            const char* arrow = cmd == CmdRight ? _ZTVN2UI16RightArrowButtonE : _ZTVN2UI15LeftArrowButtonE;
            for (int i = 0; i < int(ts.size()); i++)
                if (ts[i].owner == cur.owner && sameV(ts[i].ptr, arrow)) best = i;
        }
        return best < 0 ? from : best;
    }
    if (cmd != CmdUp && cmd != CmdDown) return from;
    float sign = cmd == CmdDown ? 1 : -1, rowDy = 1.0e9f;
    for (int i = 0; i < int(ts.size()); i++) {
        float dy = (midY(ts[i]) - midY(cur)) * sign;
        if (i != from && dy > 4 && !sameRow(cur, ts[i])) rowDy = std::min(rowDy, dy);
    }
    float bestDx = 1.0e9f;
    for (int i = 0; i < int(ts.size()); i++) {
        float dy = (midY(ts[i]) - midY(cur)) * sign, dx = fabsf(midX(ts[i]) - midX(cur));
        if (i == from || dy <= 4 || dy > rowDy + 16 || sameRow(cur, ts[i]) || dx >= bestDx) continue;
        bestDx = dx;
        best = i;
    }
    return best < 0 ? from : best;
}

UI::ChapterSelectMultiplayerWindow* s_modeWindow = nullptr;
bool s_modeWasRace = false;
int s_modeWait = 0;

void watchModeChange(const Target& t) {
    auto* m = t.owner ? one<UI::ChapterSelectMultiplayerWindow>(static_cast<UI::Window*>(t.owner),
                                                                 _ZTVN2UI30ChapterSelectMultiplayerWindowE)
                      : nullptr;
    if (!m || t.ptr != &m->m_modeScroller) return;
    if (s_modeWindow != m) s_modeWasRace = m->isRaceMode();
    s_modeWindow = m;
    s_modeWait = 0;
}

void followModeChange(const std::vector<Target>& ts) {
    UI::ChapterSelectMultiplayerWindow* m = s_modeWindow;
    if (!m) return;
    bool open = false;
    for (const Target& t : ts) open = open || t.owner == m;
    if (!open || ++s_modeWait > 120) {
        s_modeWindow = nullptr;
        return;
    }
    if (m->isRaceMode() == s_modeWasRace) return;
    s_focus = m->isRaceMode() ? static_cast<void*>(&m->m_raceScroller) : static_cast<void*>(&m->m_timedScroller);
    s_modeWindow = nullptr;
}

void applyTargets(std::vector<Target>& ts, int cmd) {
    std::sort(ts.begin(), ts.end(), [](const Target& a, const Target& b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });
    followModeChange(ts);
    int index = 0;
    for (int i = 0; i < int(ts.size()); i++)
        if (ts[i].ptr == s_focus) index = i;
    if (cmd == CmdBack) {
        for (const Target& t : ts)
            if (t.back) {
                activate(t);
                s_focus = t.ptr;
                return;
            }
        if (!ts.empty() && ts[0].owner && (ts[0].kind == KButton || ts[0].kind == KCheck))
            static_cast<UI::Window*>(ts[0].owner)->close(UI::RESULT_CANCEL);
        return;
    }
    bool alone = true;
    for (int i = 0; i < int(ts.size()) && !ts.empty(); i++)
        if (i != index && sameRow(ts[index], ts[i])) alone = false;
    if (!ts.empty() && (cmd == CmdLeft || cmd == CmdRight) && (ts[index].kind != KCheck || alone) &&
        nudge(ts[index], cmd)) {
        s_focus = ts[index].ptr;
        watchModeChange(ts[index]);
    } else if (!ts.empty() && (cmd == CmdUp || cmd == CmdDown) &&
               ((ts[index].kind == KHelp && scrollHelp(ts[index], cmd)) ||
                (ts[index].kind == KPane &&
                 scrollPane(static_cast<UI::Window*>(ts[index].ptr),
                            (cmd == CmdUp ? -1 : 1) * paneStep(static_cast<UI::Window*>(ts[index].ptr)))))) {
        s_focus = ts[index].ptr;
    } else if (!ts.empty() && (cmd == CmdLeft || cmd == CmdRight || cmd == CmdUp || cmd == CmdDown)) {
        index = nearest(ts, index, cmd);
        s_focus = ts[index].ptr;
    } else if (!ts.empty() && cmd == CmdActivate) {
        activate(ts[index]);
        s_focus = ts[index].ptr;
    } else if (!ts.empty()) {
        s_focus = ts[index].ptr;
    }
    if (ts.empty()) {
        clearLight();
        return;
    }
    for (int i = 0; i < int(ts.size()); i++)
        if (ts[i].ptr == s_focus) index = i;
    placeFrame(ts, index);
    clearLight();
}

Menu* s_zeroMenu = nullptr;
int s_zeroIndex = -1;
bool s_pauseNow = false, s_pauseBefore = false;
bool s_pauseFresh = false;
bool s_ribbonsSeen = false;

int zeroMesh() { return s_zeroMenu ? s_zeroMenu->m_subMenus[s_zeroIndex].getButtonMeshId() : -1; }

void zeroClear() {
    if (s_zeroMenu) AppEngine::getCanvas()->getQuadManager().setMeshAlpha(zeroMesh(), 1.0f);
    s_zeroMenu = nullptr;
    s_zeroIndex = -1;
}

void zeroFocus(Menu* menu, int index) {
    zeroClear();
    for (int i = 0; i < menu->m_subMenus.m_length; i++)
        if (menu->m_subMenus[i].isActive()) menu->m_subMenus[i].transitionToIdle();
    menu->m_subMenus[index].m_selected = false;
    menu->setSelectionIndex(index);
    s_zeroMenu = menu;
    s_zeroIndex = index;
}

void zeroPulse(Menu* menu) {
    if (s_zeroMenu != menu || menu->getSelectionIndex() != s_zeroIndex) {
        zeroClear();
        return;
    }
    QuadManager& quads = AppEngine::getCanvas()->getQuadManager();
    int mesh = zeroMesh();
    int w = quads.getMeshWidth(mesh);
    s_frameOn = true;
    s_frameTint = false;
    s_frame[0] = quads.getMeshRight(mesh) - w - kFrameGap;
    s_frame[1] = quads.getMeshTop(mesh) - kFrameGap;
    s_frame[2] = w + 2 * kFrameGap;
    s_frame[3] = quads.getMeshBottom(mesh) - quads.getMeshTop(mesh) + 2 * kFrameGap;
    using namespace std::chrono;
    double t = duration<double>(steady_clock::now().time_since_epoch()).count();
    float alpha = 0.55f + 0.45f * float(0.5 + 0.5 * cos(t * 6.2832 / 0.8));
    AppEngine::getCanvas()->getQuadManager().setMeshAlpha(zeroMesh(), alpha);
}

bool zeroRibbon(Menu* menu, int index) { return menu->m_subMenus[index].getLength() == 0; }

void openRibbon(Menu* menu, GameScreen* screen, int index) {
    zeroClear();
    if (sameV(menu, _ZTV8MenuMain)) static_cast<MenuMain*>(menu)->activateSubMenu(index, -1);
    else menu->activateSubMenu(index, screen, -1);
}

int focusRibbons(Menu* menu, GameScreen* screen, int cmd) {
    if (!menu->isCreated() || menu->m_subMenus.m_length <= 0) return 0;
    s_ribbonsSeen = true;
    int n = menu->m_subMenus.m_length;
    int sel = menu->getSelectionIndex();
    if (sel < 0 || sel >= n) sel = -1;
    if (screen && s_keyboard) {
        if (s_pauseFresh && zeroRibbon(menu, 0) && (sel != 0 || s_zeroMenu != menu)) {
            zeroFocus(menu, 0);
            sel = 0;
        } else if (sel >= 0 && zeroRibbon(menu, sel) && s_zeroMenu != menu) {
            zeroFocus(menu, sel);
        }
    }
    if (screen && sel >= 0 && zeroRibbon(menu, sel) && (cmd == CmdUp || cmd == CmdDown))
        cmd = cmd == CmdDown ? CmdRight : CmdLeft;
    MenuMainSubMenu* open = sel >= 0 && menu->m_subMenus[sel].isActive() ? &menu->m_subMenus[sel] : nullptr;
    if (cmd == CmdLeft || cmd == CmdRight) {
        int dir = cmd == CmdLeft ? -1 : 1;
        int next = sel < 0 ? (dir < 0 ? n - 1 : 0) : (sel + dir + n) % n;
        if (zeroRibbon(menu, next)) zeroFocus(menu, next);
        else openRibbon(menu, screen, next);
    } else if (cmd == CmdUp || cmd == CmdDown) {
        if (!open) {
            if (sel < 0 && zeroRibbon(menu, 0)) zeroFocus(menu, 0);
            else if (sel < 0) openRibbon(menu, screen, 0);
        } else if (open->getLength() > 0) {
            int cur = open->m_highlightIndex >= 0 ? open->m_highlightIndex : open->getSelectedIndex();
            int step = cmd == CmdUp ? 1 : -1;
            int nxt = cur + step;
            if (nxt < 0) nxt = 0;
            if (nxt >= open->getLength()) nxt = open->getLength() - 1;
            open->animateToSelection(nxt, false);
        }
    } else if (cmd == CmdActivate) {
        if (!open) {
            openRibbon(menu, screen, sel < 0 ? 0 : sel);
        } else if (open->getLength() == 0) {
            open->m_selected = true;
            s_arm = open;
        } else {
            int cur = open->m_highlightIndex >= 0 ? open->m_highlightIndex : open->getSelectedIndex();
            open->animateToSelection(cur, true);
            open->setSelectedIndex(cur);
            open->m_selected = true;
            s_arm = open;
        }
    } else if (cmd == CmdBack) {
        if (open) {
            open->transitionToIdle();
            menu->setSelectionIndex(-1);
        } else if (screen) {
            int host = -1;
            for (int i = 0; i < n && host < 0; i++) {
                if (menu->m_subMenus[i].m_titleId == MenuPause::ITEM_RESUME) host = i;
                for (int j = 0; j < menu->m_subMenus[i].getLength(); j++)
                    if (menu->m_subMenus[i].getItem(j) == MenuPause::ITEM_RESUME) host = i;
            }
            if (host >= 0 && zeroRibbon(menu, host)) {
                openRibbon(menu, screen, host);
            } else if (host >= 0) {
                menu->activateSubMenu(host, screen, MenuPause::ITEM_RESUME);
                MenuMainSubMenu& sub = menu->m_subMenus[host];
                sub.animateToSelection(sub.getSelectedIndex(), true);
                sub.m_selected = true;
                s_arm = &sub;
            }
        }
    }
    clearLight();
    zeroPulse(menu);
    return n;
}

void focusScreen(GameScreen* screen, int cmd, int px) {
    std::vector<Target> ts;
    switch (screen->getState()) {
    case GameScreen::SCREEN_MENU_MAIN:
        if (!s_pauseBefore) s_pauseFresh = true;
        if (cmd != 0 && cmd != CmdBack) s_pauseFresh = false;
        s_pauseNow = true;
        s_count.store(focusRibbons(&screen->m_menu, screen, cmd));
        return;
    case GameScreen::SCREEN_MULTIPLAYER_COMPLETE:
        if (screen->m_results) {
            addGame(ts, screen->m_results->netButton(0), screen, false);
            addGame(ts, screen->m_results->netButton(1), screen, false);
            addGame(ts, &screen->m_results->m_button, screen, true);
        }
        break;
    case GameScreen::SCREEN_CONFIRM_RESTART:
    case GameScreen::SCREEN_CONFIRM_QUIT:
        if (screen->m_confirmation) {
            addGame(ts, &screen->m_confirmation->m_yes, screen, false);
            addGame(ts, &screen->m_confirmation->m_no, screen, true);
        }
        break;
    case GameScreen::SCREEN_SOUND_SETTINGS:
        if (screen->m_soundSettings) {
            GameUI::SoundSettings* s = screen->m_soundSettings;
            Target music;
            music.x = s->m_music.m_left;
            music.y = s->m_music.m_y;
            music.w = s->m_music.m_right - s->m_music.m_left;
            music.h = s->m_music.getHeight();
            music.kind = KSound;
            music.ptr = &s->m_music;
            music.owner = screen;
            music.extra = 0;
            add(ts, music);
            Target fx = music;
            fx.y = s->m_effects.m_y;
            fx.h = s->m_effects.getHeight();
            fx.ptr = &s->m_effects;
            fx.extra = 1;
            add(ts, fx);
            addGame(ts, &s->m_buttons.m_button, screen, true);
        }
        break;
    case GameScreen::SCREEN_HELP_MENU:
        if (screen->m_helpText) {
            Target text;
            text.x = 40;
            text.y = 40;
            text.w = screen->getLocalWidth() - 80;
            text.h = screen->getLocalHeight() / 2;
            text.kind = KHelp;
            text.ptr = static_cast<GameUI::ScrollingText*>(screen->m_helpText);
            text.owner = screen;
            add(ts, text);
            if (px) scrollText(static_cast<GameUI::ScrollingText*>(screen->m_helpText), px);
            addGame(ts, &screen->m_helpText->m_button, screen, true);
        }
        break;
    default:
        break;
    }
    s_count.store(int(ts.size()));
    applyTargets(ts, cmd);
}

}

void menuFocusPost(int cmd) {
    s_cmd.store(cmd);
    s_keyboard = true;
}

void menuFocusPointer() { s_keyboard = false; }

void menuFocusScroll(int px, int steps) {
    s_scroll.fetch_add(px);
    s_steps.fetch_add(steps);
}

bool menuFocusActive() { return s_count.load() > 0; }

const void* menuFocusTarget() { return s_keyboard && s_frameOn && s_count.load() > 0 ? s_focus : nullptr; }
void menuFocusSet(const void* target) { s_focus = const_cast<void*>(target); }

void menuFocusApplyFrame();

void menuFocusApply() {
    s_ribbonsSeen = false;
    menuFocusApplyFrame();
    if (!s_ribbonsSeen) zeroClear();
}

void menuFocusApplyFrame() {
    if (s_arm) {
        s_arm->m_selected = false;
        s_arm = nullptr;
    }
    s_userRow = -1;
    s_pauseBefore = s_pauseNow;
    s_pauseNow = false;
    int cmd = s_cmd.exchange(0);
    int px = s_scroll.exchange(0), steps = s_steps.exchange(0);
    s_frameOn = false;
    AppEngine* app = AppEngine::getCanvas();
    if (!app) {
        s_count.store(0);
        clearLight();
        return;
    }
    UI::WindowStore& store = app->getWindowStore();
    UI::Window* top = nullptr;
    pthread_mutex_lock(&store.m_windowsLock);
    for (int i = int(store.m_windows.size()) - 1; i >= 0; --i) {
        if (store.m_windows[i] && !store.m_windows[i]->isClosed()) {
            top = store.m_windows[i];
            break;
        }
    }
    pthread_mutex_unlock(&store.m_windowsLock);
    if (top) {
        std::vector<Target> ts;
        collectWindow(top, ts);
        if (px || steps) scrollWindow(top, ts, px, steps);
        s_count.store(int(ts.size()));
        applyTargets(ts, cmd);
        showUserRow(top, cmd != 0);
        return;
    }
    if (SceneMenu* menu = app->getSceneMenu()) {
        if (menu->m_state == SceneMenu::STATE_MAIN_MENU || menu->m_state == SceneMenu::STATE_MAIN_MENU_ALT) {
            s_count.store(focusRibbons(&menu->m_mainMenu, nullptr, cmd));
            return;
        }
    }
    if (SceneGame* game = app->getSceneGame()) {
        if (game->m_gameScreens.m_length > 0) {
            focusScreen(game->getGameScreen(0), cmd, px);
            return;
        }
    }
    s_count.store(0);
    s_focus = nullptr;
    clearLight();
}

}
}

extern "C" void medecomp_menu_focus_draw(midp::Graphics* g) {
    using namespace port::platform;
    if (!s_keyboard || !s_frameOn || s_count.load() == 0) return;
    int cx = g->getClipX(), cy = g->getClipY(), cw = g->getClipWidth(), ch = g->getClipHeight();
    AppEngine* app = AppEngine::getCanvas();
    g->setClip(0, 0, app->getWidth(), app->getHeight());
    int x = s_frame[0], y = s_frame[1], w = s_frame[2], h = s_frame[3];
    const int t = 3, kTint = 20;
    int arm = std::max(8, std::min(22, std::min(w, h) / 3));
    if (s_frameTint) {
        g->setColor(kTint, 0, 0, kTint);
        g->fillRect(x, y, w, h);
    }
    g->setColor(255, 0, 0, 255);
    for (int corner = 0; corner < 4; corner++) {
        int ax = corner & 1 ? x + w - arm : x, ay = corner & 2 ? y + h - t : y;
        int bx = corner & 1 ? x + w - t : x, by = corner & 2 ? y + h - arm : y;
        g->fillRect(ax, ay, arm, t);
        g->fillRect(bx, by, t, arm);
    }
    g->setClip(cx, cy, cw, ch);
}

extern "C" int medecomp_challenge_focus_row(void) { return port::platform::s_userRow; }
