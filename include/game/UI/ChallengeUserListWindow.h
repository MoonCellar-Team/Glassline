#pragma once

#include "ea_online/Mayhem/GetUserListRequest.h"
#include "game/UI/Window.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"

namespace UI {

class ChallengeUserList;
class ChallengeUserWindow;

class ChallengeUserListWindow : public Window {
public:
    ChallengeUserListWindow(ChallengeUserWindow& owner, int x, int y, int width, int height);
    virtual ~ChallengeUserListWindow();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);

    void userListFinishedLoading();

    ChallengeUserWindow* m_owner;
    ChallengeUserList* m_list;
    midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest> m_request;
};

}
