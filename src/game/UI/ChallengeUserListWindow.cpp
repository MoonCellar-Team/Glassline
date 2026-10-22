#include <stdint.h>
#include "game/UI/ChallengeUserListWindow.h"

#include <vector>

#include "ea_online/FBConnection.h"
#include "ea_online/Mayhem/Request.h"
#include "game/AppEngine.h"
#include "game/UI/ChallengeUserList.h"
#include "game/UI/NetworkWaitEffect.h"
#include "runtime/midp/Thread.h"

namespace UI {

void ChallengeUserListWindow::update(int dt) {
    Window::update(dt);
    if (m_list == NULL) {
        Mayhem::GetUserListRequest* request = m_request;
        Mayhem::Request* result = request;
        if (result->GetResult() == Mayhem::Request::RESULT_SUCCESS) {
            userListFinishedLoading();
        } else {
            midp::Thread::yield();
        }
        if (m_list == NULL) {
            return;
        }
    }
    m_list->update(dt);
}

void ChallengeUserListWindow::render(midp::Graphics* g, int originY, int originX) {
    if (m_list != NULL) {
        Window::render(g, static_cast<int>(reinterpret_cast<uintptr_t>(m_list)), originX);
    }
}

ChallengeUserListWindow::ChallengeUserListWindow(ChallengeUserWindow& owner, int x, int y, int width,
                                                int height)
    : Window(x, y, width, height),
      m_owner(&owner),
      m_list(NULL),
      m_request(NULL) {
    std::vector<long long> friendIds;
    FBConnection* connection = 0;
    connection->GetFriendIDs(friendIds);
    Mayhem::GetUserListRequest* request = new Mayhem::GetUserListRequest(friendIds);
    midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest> held(request);
    m_request = held;
    AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->play(x + (width >> 1),
                                                                           y + (height >> 1));
}

void ChallengeUserListWindow::userListFinishedLoading() {
    midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest> request(m_request);
    m_list = new ChallengeUserList(*m_owner, request, m_clientWidth - 0x20);
    addElement(m_list);
    AppEngine::getCanvas()->getWindowStore().getNetworkWaitEffect()->stop();
}

ChallengeUserListWindow::~ChallengeUserListWindow() {}

}
