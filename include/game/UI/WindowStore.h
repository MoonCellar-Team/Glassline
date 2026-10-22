#pragma once

#include <pthread.h>
#include <vector>

#include "game/UI/ButtonEffect.h"
#include "game/UI/NetworkWaitEffect.h"

namespace midp {
class Graphics;
}

namespace UI {

class ButtonEffect;
class NetworkWaitEffect;
class ButtonEffect;
class Window;

class WindowStore {
public:
    enum WindowType {
        WINDOW_PLAIN = 0,
        WINDOW_WITH_BACKGROUND = 1,
        WINDOW_GAME_OPTIONS = 2,
        WINDOW_SOUND_OPTIONS = 3,
        WINDOW_ACHIEVEMENTS = 4,
        WINDOW_STORY_END_OF_LEVEL_PROMPT = 7,
        WINDOW_SPEEDRUN_END_OF_LEVEL_PROMPT = 8,
        WINDOW_CHALLENGE_END_OF_LEVEL_PROMPT = 9,
        WINDOW_END_OF_GAME_RESULTS = 10,
        WINDOW_ABOUT_MENU = 11,
        WINDOW_ABOUT = 12,
        WINDOW_HELP = 13,
        WINDOW_UNLOCKABLES = 14,
        WINDOW_LANGUAGE = 15,
        WINDOW_EXIT_TO_BROWSER_CONFIRM = 18,
        WINDOW_CHAPTER_SELECT_STORY = 19,
        WINDOW_CHAPTER_SELECT_SPEEDRUN = 20,
        WINDOW_CHAPTER_SELECT_MULTIPLAYER = 21,
        WINDOW_NEW_GAME_CONFIRM = 22,
        WINDOW_TUTORIAL_CONFIRM = 23,
        WINDOW_CHALLENGE_CONFIRM = 24,
        WINDOW_ENABLE_SHARE_DATA_CONFIRM = 25,
        WINDOW_DISABLE_SHARE_DATA_CONFIRM = 26,
        WINDOW_SHARE_DATA_ENABLED_CONFIRM = 27,
        WINDOW_SHARE_DATA_DISABLED_CONFIRM = 28,
        WINDOW_LEADERBOARD = 29,
        WINDOW_UPLOAD_CONFIRMATION = 30,
        WINDOW_DISPLAY_NAME_ENTRY = 31,
        WINDOW_PLEASE_WAIT = 32,
        WINDOW_CHALLENGE_USER = 33,
        WINDOW_CHALLENGE_COMMENT_ENTRY = 34,
        WINDOW_NO_STORAGE = 35,
    };

    WindowStore();
    ~WindowStore();

    NetworkWaitEffect* getNetworkWaitEffect();
    ButtonEffect* getButtonEffect();
    int getWindowResult() const;
    void clearWindowResult();
    void setWaitingForMainThread();
    void unsetWaitingForMainThread();

    void pointerDragged(int x, int y, int id);
    void pointerReleased(int x, int y, int id);
    void pointerPressed(int x, int y, int id);
    void render(midp::Graphics* g);
    void update(int dt);
    Window* pushWindow(Window* window);
    Window* createWindow(WindowType type, int x, int y, int width, int height);

    bool onDeleteWindow(Window* window);

    int m_windowResult;
    std::vector<Window*> m_windows;
    std::vector<Window*> m_pendingWindows;
    ButtonEffect m_buttonEffect;
    NetworkWaitEffect m_networkWaitEffect;
    pthread_mutex_t m_windowsLock;
    pthread_mutex_t m_pendingLock;
    int m_waitingForMainThread;
};

}
