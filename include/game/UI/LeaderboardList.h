#pragma once

#include <string>
#include <vector>

#include "ea_online/Mayhem/GetLeaderboardRequest.h"
#include "ea_online/Mayhem/GetStatRequest.h"
#include "game/UI/WindowElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"

namespace midp {
class Graphics;
class Image;
}

namespace UI {

class LeaderboardWindow;

class LeaderboardList : public WindowElement {
public:
    static const int MAX_ENTRIES;

    LeaderboardList(LeaderboardWindow& window, int levelIndex,
                    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> request, int width);
    virtual ~LeaderboardList();

    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerReleased(int x, int y, int id);

    int cache();
    void uncache();

    LeaderboardWindow* m_window;
    midp::ReferenceCountedPointer<midp::Image> m_image;
    int m_levelIndex;
    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> m_request;
    int m_selected;
    bool m_effectPlayed;
    std::vector<std::string> m_statIds;
    midp::SafeReferenceCountedPointer<Mayhem::GetStatRequest> m_statRequest;
};

}
