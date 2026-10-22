#include "game/UI/WindowStore.h"

#include <algorithm>
#include <functional>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

#include "game/UI/AboutMenu.h"
#include "game/UI/AboutWindow.h"
#include "game/UI/AchievementWindow.h"
#include "game/UI/ChallengeCommentEntry.h"
#include "game/UI/ChallengeConfirm.h"
#include "game/UI/ChallengeEndOfLevelPrompt.h"
#include "game/UI/ChallengeUserWindow.h"
#include "game/UI/ChapterSelectMultiplayerWindow.h"
#include "game/UI/ChapterSelectSpeedrunWindow.h"
#include "game/UI/ChapterSelectStoryWindow.h"
#include "game/UI/DisableShareDataConfirm.h"
#include "game/UI/DisplayNameEntry.h"
#include "game/UI/EnableShareDataConfirm.h"
#include "game/UI/EndOfGameResultsWindow.h"
#include "game/UI/ExitToBrowserConfirm.h"
#include "game/UI/GameOptions.h"
#include "game/UI/HelpWindow.h"
#include "game/UI/LanguageWindow.h"
#include "game/UI/LeaderboardWindow.h"
#include "game/UI/NewGameConfirm.h"
#include "game/UI/NoStorage.h"
#include "game/UI/PleaseWaitWindow.h"
#include "game/UI/ShareDataDisabledConfirm.h"
#include "game/UI/ShareDataEnabledConfirm.h"
#include "game/UI/SoundOptions.h"
#include "game/UI/SpeedRunEndOfLevelPrompt.h"
#include "game/UI/StoryEndOfLevelPrompt.h"
#include "game/UI/TutorialConfirm.h"
#include "game/UI/UnlockableWindow.h"
#include "game/UI/UploadConfirmationWindow.h"
#include "game/UI/Window.h"
#include "game/UI/WindowWithBackground.h"

namespace UI {

NetworkWaitEffect* WindowStore::getNetworkWaitEffect() {
    return &m_networkWaitEffect;
}

ButtonEffect* WindowStore::getButtonEffect() {
    return &m_buttonEffect;
}

WindowStore::WindowStore() : m_windowResult(0) {
    memset(&m_windowsLock, 0, sizeof(WindowStore) - offsetof(WindowStore, m_windowsLock));
    pthread_mutex_init(&m_windowsLock, NULL);
    pthread_mutex_init(&m_pendingLock, NULL);
}

bool WindowStore::onDeleteWindow(Window* window) {
    if (!window->isClosed()) {
        return false;
    }
    m_windowResult = window->getWindowResult();
    if (window->deleteOnClosed()) {
        delete window;
    }
    return true;
}

int WindowStore::getWindowResult() const {
    return m_windowResult;
}

void WindowStore::clearWindowResult() {
    m_windowResult = 0;
}

void WindowStore::setWaitingForMainThread() {
    if (m_waitingForMainThread == 0) {
        __sync_fetch_and_add(&m_waitingForMainThread, 1);
    }
}

void WindowStore::unsetWaitingForMainThread() {
    if (m_waitingForMainThread != 0) {
        __sync_fetch_and_sub(&m_waitingForMainThread, 1);
    }
}

void WindowStore::pointerDragged(int x, int y, int id) {
    if (m_waitingForMainThread != 0) {
        return;
    }
    pthread_mutex_lock(&m_windowsLock);
    std::vector<Window*>::const_iterator it;
    std::vector<Window*>::const_iterator end = m_windows.end();
    for (it = m_windows.begin(); it != end; ++it) {
        Window* window = *it;
        if (window->contains(x, y)) {
            window->pointerDragged(x - window->getX(), y - window->getY(), id);
        }
    }
    pthread_mutex_unlock(&m_windowsLock);
}

void WindowStore::pointerReleased(int x, int y, int id) {
    if (m_waitingForMainThread != 0) {
        return;
    }
    pthread_mutex_lock(&m_windowsLock);
    std::vector<Window*>::const_iterator it;
    std::vector<Window*>::const_iterator end = m_windows.end();
    for (it = m_windows.begin(); it != end; ++it) {
        Window* window = *it;
        if (window->contains(x, y)) {
            window->pointerReleased(x - window->getX(), y - window->getY(), id);
        }
    }
    pthread_mutex_unlock(&m_windowsLock);
}

void WindowStore::pointerPressed(int x, int y, int id) {
    if (m_waitingForMainThread != 0) {
        return;
    }
    pthread_mutex_lock(&m_windowsLock);
    std::vector<Window*>::const_iterator it;
    std::vector<Window*>::const_iterator end = m_windows.end();
    for (it = m_windows.begin(); it != end; ++it) {
        Window* window = *it;
        if (window->contains(x, y)) {
            window->pointerPressed(x - window->getX(), y - window->getY(), id);
        }
    }
    pthread_mutex_unlock(&m_windowsLock);
}

void WindowStore::render(midp::Graphics* g) {
    pthread_mutex_lock(&m_windowsLock);
    std::for_each(m_windows.begin(), m_windows.end(), std::bind2nd(std::mem_fun(&Window::render), g));
    pthread_mutex_unlock(&m_windowsLock);
}

void WindowStore::update(int dt) {
    pthread_mutex_lock(&m_windowsLock);
    std::for_each(m_windows.begin(), m_windows.end(), std::bind2nd(std::mem_fun(&Window::update), dt));
    m_windows.erase(std::remove_if(m_windows.begin(), m_windows.end(),
                                   std::bind1st(std::mem_fun(&WindowStore::onDeleteWindow), this)),
                    m_windows.end());
    pthread_mutex_lock(&m_pendingLock);
    m_windows.insert(m_windows.end(), m_pendingWindows.begin(), m_pendingWindows.end());
    m_pendingWindows.clear();
    pthread_mutex_unlock(&m_pendingLock);
    m_buttonEffect.update(dt);
    m_networkWaitEffect.update(dt);
    pthread_mutex_unlock(&m_windowsLock);
}

Window* WindowStore::pushWindow(Window* window) {
    pthread_mutex_lock(&m_pendingLock);
    m_pendingWindows.push_back(window);
    pthread_mutex_unlock(&m_pendingLock);
    return window;
}

Window* WindowStore::createWindow(WindowType type, int x, int y, int width, int height) {
    Window* window = NULL;
    switch (type) {
    case WINDOW_PLAIN:
        window = new Window(x, y, width, height);
        break;
    case WINDOW_WITH_BACKGROUND:
        window = new WindowWithBackground(x, y, width, height);
        break;
    case WINDOW_GAME_OPTIONS:
        window = new GameOptions();
        break;
    case WINDOW_SOUND_OPTIONS:
        window = new SoundOptions();
        break;
    case WINDOW_ACHIEVEMENTS:
        window = new AchievementWindow();
        break;
    case WINDOW_LEADERBOARD:
        window = new LeaderboardWindow();
        break;
    case WINDOW_STORY_END_OF_LEVEL_PROMPT:
        window = new StoryEndOfLevelPrompt();
        break;
    case WINDOW_SPEEDRUN_END_OF_LEVEL_PROMPT:
        window = new SpeedRunEndOfLevelPrompt();
        break;
    case WINDOW_CHALLENGE_END_OF_LEVEL_PROMPT:
        window = new ChallengeEndOfLevelPrompt();
        break;
    case WINDOW_END_OF_GAME_RESULTS:
        window = new EndOfGameResultsWindow();
        break;
    case WINDOW_ABOUT_MENU:
        window = new AboutMenu();
        break;
    case WINDOW_ABOUT:
        window = new AboutWindow();
        break;
    case WINDOW_HELP:
        window = new HelpWindow();
        break;
    case WINDOW_UNLOCKABLES:
        window = new UnlockableWindow();
        break;
    case WINDOW_LANGUAGE:
        window = new LanguageWindow();
        break;
    case WINDOW_EXIT_TO_BROWSER_CONFIRM:
        window = new ExitToBrowserConfirm();
        break;
    case WINDOW_CHAPTER_SELECT_STORY:
        window = new ChapterSelectStoryWindow();
        break;
    case WINDOW_CHAPTER_SELECT_SPEEDRUN:
        window = new ChapterSelectSpeedrunWindow();
        break;
    case WINDOW_CHAPTER_SELECT_MULTIPLAYER:
        window = new ChapterSelectMultiplayerWindow();
        break;
    case WINDOW_NEW_GAME_CONFIRM:
        window = new NewGameConfirm();
        break;
    case WINDOW_TUTORIAL_CONFIRM:
        window = new TutorialConfirm();
        break;
    case WINDOW_CHALLENGE_CONFIRM:
        window = new ChallengeConfirm();
        break;
    case WINDOW_ENABLE_SHARE_DATA_CONFIRM:
        window = new EnableShareDataConfirm();
        break;
    case WINDOW_DISABLE_SHARE_DATA_CONFIRM:
        window = new DisableShareDataConfirm();
        break;
    case WINDOW_SHARE_DATA_ENABLED_CONFIRM:
        window = new ShareDataEnabledConfirm();
        break;
    case WINDOW_SHARE_DATA_DISABLED_CONFIRM:
        window = new ShareDataDisabledConfirm();
        break;
    case WINDOW_UPLOAD_CONFIRMATION:
        window = new UploadConfirmationWindow();
        break;
    case WINDOW_DISPLAY_NAME_ENTRY:
        window = new DisplayNameEntry();
        break;
    case WINDOW_PLEASE_WAIT:
        window = new PleaseWaitWindow();
        break;
    case WINDOW_CHALLENGE_USER:
        window = new ChallengeUserWindow();
        break;
    case WINDOW_CHALLENGE_COMMENT_ENTRY:
        window = new ChallengeCommentEntry();
        break;
    case WINDOW_NO_STORAGE:
        window = new NoStorage();
        break;
    }
    if (window != NULL) {
        pushWindow(window);
    }
    return window;
}

WindowStore::~WindowStore() {
    std::vector<Window*>::const_iterator it;
    std::vector<Window*>::const_iterator end = m_windows.end();
    for (it = m_windows.begin(); it != end; ++it) {
        delete *it;
    }
    pthread_mutex_destroy(&m_windowsLock);
    pthread_mutex_destroy(&m_pendingLock);
}

}
