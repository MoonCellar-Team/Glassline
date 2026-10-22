#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <string>

#include "game/AppEngine.h"
#include "game/SceneGame.h"
#include "game/SceneMenu.h"
#include "port/files.h"
#include "port/platform.h"

namespace {

using Clock = std::chrono::steady_clock;

const char* menuState(int s) {
    switch (s) {
    case SceneMenu::STATE_LOADING: return "загрузка";
    case SceneMenu::STATE_TITLE: return "заставка";
    case SceneMenu::STATE_MAIN_MENU:
    case SceneMenu::STATE_MAIN_MENU_ALT: return "главное";
    case SceneMenu::STATE_LEVEL_SELECT_STORY: return "выбор главы";
    case SceneMenu::STATE_LEVEL_SELECT_SPEEDRUN: return "выбор уровня на время";
    case SceneMenu::STATE_LEVEL_SELECT_MULTIPLAYER: return "выбор уровня вдвоём";
    case SceneMenu::STATE_GAME_INTRO: return "вступление";
    case SceneMenu::STATE_SOUND_OPTIONS: return "звук";
    case SceneMenu::STATE_GAME_OPTIONS: return "настройки";
    case SceneMenu::STATE_ACHIEVEMENTS: return "значки";
    case SceneMenu::STATE_HELP: return "справка";
    case SceneMenu::STATE_ABOUT_MENU: return "об игре";
    case SceneMenu::STATE_ABOUT: return "об авторах";
    case SceneMenu::STATE_LEADERBOARD_WAIT: return "ожидание таблицы";
    case SceneMenu::STATE_LEADERBOARD: return "таблица лидеров";
    case SceneMenu::STATE_UNLOCKABLES: return "обои";
    case SceneMenu::STATE_LANGUAGE: return "язык";
    case SceneMenu::STATE_TRANSITION_TO_GAME: return "переход в игру";
    case SceneMenu::STATE_NEW_GAME_CONFIRM: return "новая игра?";
    case SceneMenu::STATE_TUTORIAL_CONFIRM: return "обучение?";
    default: return nullptr;
    }
}

const char* gameState(int s) {
    switch (s) {
    case SceneGame::STATE_LOADING: return "загрузка";
    case SceneGame::STATE_INTRO_CAM: return "облёт камеры";
    case SceneGame::STATE_RACE_START: return "отсчёт";
    case SceneGame::STATE_GAME: return "забег";
    case SceneGame::STATE_FADE_OUT: return "затемнение";
    case SceneGame::STATE_LEVEL_COMPLETE_SINGLE:
    case SceneGame::STATE_LEVEL_COMPLETE_MULTIPLAYER: return "уровень пройден";
    case SceneGame::STATE_MENU_PAUSED: return "пауза";
    case SceneGame::STATE_POPUP_BOX: return "подсказка";
    case SceneGame::STATE_CAMERA_ANIM: return "ролик камеры";
    default: return nullptr;
    }
}

std::string screenName() {
    AppEngine* app = AppEngine::getCanvas();
    if (!app) return "запуск";
    char buf[96];
    if (app->m_sceneId == 2) {
        SceneMenu* menu = app->getSceneMenu();
        if (!menu) return "меню";
        const char* name = menuState(menu->m_state);
        if (name) return std::string("меню: ") + name;
        snprintf(buf, sizeof buf, "меню: состояние %d", int(menu->m_state));
        return buf;
    }
    if (app->m_sceneId == 3) {
        SceneGame* game = app->getSceneGame();
        if (!game) return "уровень";
        if (game->m_loadingStep < 13) {
            snprintf(buf, sizeof buf, "уровень: загрузка, шаг %d", game->m_loadingStep);
            return buf;
        }
        const char* name = gameState(game->getState());
        if (name) return std::string("уровень: ") + name;
        snprintf(buf, sizeof buf, "уровень: состояние %d", game->getState());
        return buf;
    }
    snprintf(buf, sizeof buf, "сцена %d", app->m_sceneId);
    return buf;
}

FILE* s_file;
bool s_opened;
Clock::time_point s_start = Clock::now();

bool verbose() {
    static const bool on = getenv("MEDECOMP_PERF_LOG") != nullptr;
    return on;
}

double slowMs() {
    static const double ms = getenv("MEDECOMP_PERF_SLOW") ? atof(getenv("MEDECOMP_PERF_SLOW")) : 100.0;
    return ms;
}

void out(bool alsoStderr, const char* fmt, const std::string& a, const std::string& b, double x, double y, int n) {
    char line[512];
    snprintf(line, sizeof line, fmt, a.c_str(), b.c_str(), x, y, n);
    if (!s_opened) {
        s_opened = true;
        s_file = fopen((port::files::dataDir() + "/perf.log").c_str(), "w");
    }
    double t = std::chrono::duration<double>(Clock::now() - s_start).count();
    if (s_file) {
        fprintf(s_file, "%9.3f %s\n", t, line);
        fflush(s_file);
    }
    if (alsoStderr) fprintf(stderr, "medecomp: %s\n", line);
}

std::string s_screen;
Clock::time_point s_since;
int s_frames;
double s_longest;

}

namespace port {
namespace platform {

void perfFrame(double workMs) {
    std::string screen = screenName();
    Clock::time_point now = Clock::now();
    if (screen != s_screen) {
        if (verbose() && !s_screen.empty()) {
            double stayed = std::chrono::duration<double, std::milli>(now - s_since).count();
            out(false, "экран «%s» → «%s»: %.0f мс, самый долгий кадр %.0f мс, кадров %d", s_screen, screen, stayed,
                s_longest, s_frames);
        }
        s_screen = screen;
        s_since = now;
        s_frames = 0;
        s_longest = 0;
    }
    s_frames++;
    if (workMs > s_longest) s_longest = workMs;
    if (workMs >= 1000.0)
        out(true, "аномально долгий кадр: %s%s%.0f мс", "«" + screen + "» — ", "", workMs, 0, 0);
    else if (verbose() && workMs >= slowMs())
        out(false, "долгий кадр: %s%s%.0f мс", "«" + screen + "» — ", "", workMs, 0, 0);
}

}
}
