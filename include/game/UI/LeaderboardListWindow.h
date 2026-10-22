#pragma once

#include <vector>

#include "ea_online/Mayhem/GetLeaderboardRequest.h"
#include "game/UI/Window.h"
#include "game/WrappedString.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"

namespace UI {

class LeaderboardList;
class LeaderboardWindow;

class LeaderboardListWindow : public Window {
public:
    LeaderboardListWindow(LeaderboardWindow& window, int x, int y, int width, int height);
    virtual ~LeaderboardListWindow();

    virtual void update(int dt);
    using Window::render;
    virtual void render(midp::Graphics* g, int originY, int originX);

    void friendboardFinishedLoading(int index);
    void leaderboardFinishedLoading(int index);
    void SetListIdx(int index, bool friends);

    LeaderboardWindow* m_window;
    std::vector<LeaderboardList*> m_lists;
    std::vector<LeaderboardList*> m_friendLists;
    std::vector<midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>> m_requests;
    std::vector<midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest>> m_friendRequests;
    int m_index;
    int m_friends;
    bool m_failed;
    WrappedString m_error;
};

}
