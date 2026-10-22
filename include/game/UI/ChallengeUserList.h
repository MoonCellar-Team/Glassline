#pragma once

#include <string>
#include <vector>

#include "ea_online/Mayhem/UserRequest.h"
#include "game/UI/WindowElement.h"
#include "runtime/midp/ReferenceCountedPointer.h"
#include "runtime/midp/SafeReferenceCountedPointer.h"

namespace midp {
class Image;
class WebImage;
}
namespace Mayhem {
class GetUserListRequest;
}
namespace UI {

class ChallengeUserWindow;

class ChallengeUserList : public WindowElement {
public:
    ChallengeUserList(ChallengeUserWindow& owner,
                      midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest> request,
                      int width);
    virtual ~ChallengeUserList();

    virtual void update(int dt);
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerReleased(int x, int y, int id);

    ChallengeUserWindow* m_owner;
    midp::ReferenceCountedPointer<midp::Image> m_listImage;
    std::vector<midp::ReferenceCountedPointer<midp::WebImage>> m_avatars;
    int m_selected;
    bool m_effectPlayed;
    unsigned char m_pad35[3];
    std::vector<std::string> m_userIds;
    midp::SafeReferenceCountedPointer<Mayhem::UserRequest> m_userRequest;
};

}
