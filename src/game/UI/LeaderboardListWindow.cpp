#include "game/UI/LeaderboardListWindow.h"

#include <stdio.h>

#include "ea_online/FBConnection.h"
#include "ea_online/Mayhem/Request.h"
#include "game/AppEngine.h"
#include "game/TextManager.h"
#include "game/UI/LeaderboardList.h"
#include "game/UI/LeaderboardWindow.h"
#include "game/UI/NetworkWaitEffect.h"
#include "runtime/midp/Thread.h"

static const int TEXT_ERROR = 0x976;
static const int TEXT_FOOTER = 0x96b;
static const int FONT_ERROR = 2;
static const int FONT_FOOTER = 9;
static const int ANCHOR_ERROR = 0x12;
static const int ANCHOR_FOOTER = 0x41;
static const int ERROR_Y = 0x1ba;
static const int FOOTER_X = 0x1e;
static const int FOOTER_Y_CUT = 0x32;
static const int STAT_NAME_SIZE = 20;
static const char* const STAT_NAME = "highscore_level%d";

namespace UI {

static void clearRequest(midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>& slot) {
    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> empty(NULL);
    slot = empty;
}

static void showWait(AppEngine* canvas) {
    UI::NetworkWaitEffect* effect = canvas->getWindowStore().getNetworkWaitEffect();
    effect->play(canvas->getWidth() >> 1, canvas->getHeight() >> 1);
}

LeaderboardListWindow::~LeaderboardListWindow() {
    if (m_lists.size() == 0) {
        return;
    }
    unsigned index = 0;
    do {
        LeaderboardList* list = m_lists[index];
        WindowElement* shown = getElement(0);
        if (list != NULL && list != shown) {
            delete list;
        }
        LeaderboardList* friends = m_friendLists[index];
        shown = getElement(0);
        if (friends != NULL && friends != shown) {
            delete friends;
        }
        ++index;
    } while (index < m_lists.size());
}

void LeaderboardListWindow::update(int dt) {
    Window::update(dt);
    if (m_requests.size() == 0) {
        return;
    }
    unsigned index = 0;
    do {
        LeaderboardList*& list = m_lists[index];
        if (list == NULL) {
            midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>& request = m_requests[index];
            if (request != NULL) {
                if (request->GetResult() == Mayhem::Request::RESULT_PENDING) {
                    midp::Thread::yield();
                } else {
                    leaderboardFinishedLoading(index);
                }
            }
        }
        LeaderboardList*& friends = m_friendLists[index];
        if (friends == NULL) {
            midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>& request = m_friendRequests[index];
            if (request != NULL) {
                if (request->GetResult() == Mayhem::Request::RESULT_PENDING) {
                    midp::Thread::yield();
                } else {
                    friendboardFinishedLoading(index);
                }
            }
        }
        ++index;
    } while (index < m_requests.size());
}

void LeaderboardListWindow::render(midp::Graphics* g, int originY, int originX) {
    if (m_failed) {
        m_error.draw(g, originX + (m_width >> 1), originY + ERROR_Y, ANCHOR_ERROR);
        return;
    }
    std::vector<LeaderboardList*>& lists = m_friends != 0 ? m_friendLists : m_lists;
    if (lists[m_index] == NULL) {
        return;
    }
    Window::render(g, originY, originX);
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    text->drawString(g, TEXT_FOOTER, FONT_FOOTER, FOOTER_X, canvas->getHeight() - FOOTER_Y_CUT, ANCHOR_FOOTER);
}

void LeaderboardListWindow::friendboardFinishedLoading(int index) {
    int result = m_requests[index]->GetResult();
    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>& request = m_friendRequests[index];
    if (result == Mayhem::Request::RESULT_SUCCESS) {
        LeaderboardList* list = new LeaderboardList(*m_window, index, request, m_clientWidth - 0x20);
        m_friendLists[index] = list;
        if (m_index == index && m_friends != 0) {
            addElement(list);
        }
    } else {
        clearRequest(request);
        m_failed = true;
        setClientOffsetY(0);
        adjustClientArea();
    }
    AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->stop();
}

void LeaderboardListWindow::leaderboardFinishedLoading(int index) {
    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>& request = m_requests[index];
    int result = request->GetResult();
    if (result == Mayhem::Request::RESULT_SUCCESS) {
        LeaderboardList* list = new LeaderboardList(*m_window, index, request, m_clientWidth - 0x20);
        m_lists[index] = list;
        if (m_index == index && m_friends == 0) {
            addElement(list);
        }
    } else {
        clearRequest(request);
        m_failed = true;
        setClientOffsetY(0);
        adjustClientArea();
    }
    AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->stop();
}

void LeaderboardListWindow::SetListIdx(int index, bool friends) {
    AppEngine* canvas = AppEngine::getCanvas();
    WindowElement* shown = getElement(0);
    if (shown != NULL) {
        static_cast<LeaderboardList*>(shown)->uncache();
        removeElement(shown);
    }
    m_contentHeight = 0;
    m_index = index;
    m_friends = friends;
    m_failed = false;
    if (friends) {
        LeaderboardList* list = m_friendLists[index];
        if (list != NULL) {
            addElement(list);
        } else if (m_friendRequests[index] == NULL) {
            char statName[STAT_NAME_SIZE];
            snprintf(statName, sizeof(statName), STAT_NAME, index + 1);
            std::vector<long long> friendIds;
            AppEngine::getCanvas();
            FBConnection* connection = NULL;
            connection->GetFriendIDs(friendIds);
            Mayhem::Token* token = canvas->m_mayhemToken.unsafeGetWrappedPointer();
            {
                std::string statId(statName);
                midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> request(
                    new Mayhem::GetLeaderboardRequest(token, statId, friendIds));
                m_friendRequests[index] = request;
            }
            showWait(canvas);
        }
    } else {
        LeaderboardList* list = m_lists[index];
        if (list != NULL) {
            addElement(list);
        } else if (m_requests[index] == NULL) {
            char statName[STAT_NAME_SIZE];
            snprintf(statName, sizeof(statName), STAT_NAME, index + 1);
            Mayhem::Token* token = canvas->m_mayhemToken.unsafeGetWrappedPointer();
            {
                std::string statId(statName);
                midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> request(
                    new Mayhem::GetLeaderboardRequest(token, statId, LeaderboardList::MAX_ENTRIES));
                m_requests[index] = request;
            }
            showWait(canvas);
        }
    }
    setClientOffsetY(0);
    adjustClientArea();
}

LeaderboardListWindow::LeaderboardListWindow(LeaderboardWindow& window, int x, int y, int width, int height)
    : Window(x, y, width, height),
      m_window(&window),
      m_lists(),
      m_friendLists(),
      m_requests(),
      m_friendRequests(),
      m_index(0),
      m_friends(0),
      m_failed(false),
      m_error() {
    int count = m_window->getItemCount();
    m_lists.resize(count, NULL);
    m_friendLists.resize(count, NULL);
    {
        midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> empty;
        m_requests.resize(count, empty);
    }
    {
        midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> empty;
        m_friendRequests.resize(count, empty);
    }
    if (count >= 1) {
        for (int i = 0; i != count; ++i) {
            clearRequest(m_requests[i]);
            clearRequest(m_friendRequests[i]);
        }
    }
    m_error.wrapString(TEXT_ERROR, FONT_ERROR, m_width - 0x1e);
    SetListIdx(0, false);
}

}
