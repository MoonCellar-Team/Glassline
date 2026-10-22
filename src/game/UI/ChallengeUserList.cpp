#include "game/UI/ChallengeUserList.h"

#include "ea_online/FBConnection.h"
#include "ea_online/FBFriend.h"
#include "ea_online/Mayhem/UserList.h"
#include "game/AppEngine.h"
#include "game/SceneGame.h"
#include "game/TextManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/ChallengeUserWindow.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"

static const int ROW_HEIGHT = 0x38;
static const int FONT_NAME = 2;
static const int ANCHOR_ROW = 9;
static const int ANCHOR_NAME = 0x11;
static const int COLOR_STRIPE = 0x7fd4e8ff;
static const int COLOR_PLAIN = 0x7fffffff;

#ifdef MEDECOMP_PORT
extern "C" int medecomp_challenge_focus_row(void);
#endif

namespace {

Mayhem::UserList* asUserList(midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest>& request) {
    return request.operator->();
}

}

namespace UI {

static const WindowResult RESULT_CHOSEN = static_cast<WindowResult>(3);

void ChallengeUserList::update(int dt) {
    (void)dt;
}

void ChallengeUserList::render(midp::Graphics* g, int originY, int originX) {
    AppEngine* engine = AppEngine::getCanvas();
    if (m_listImage != NULL) {
        bool hide = false;
        if (m_effectPlayed) {
            hide = engine->getWindowStore().getButtonEffect()->isAnimating();
        }
        if (!hide) {
            g->drawImage(m_listImage, originX, originY, ANCHOR_ROW);
            if (m_avatars.size() != 0) {
                int avatarY = originY + 3;
                for (unsigned i = 0; i < m_avatars.size(); i++) {
                    if (m_avatars[i] != NULL && m_avatars[i]->GetResult() == midp::WebImage::kSuccess) {
                        g->drawImage(m_avatars[i], originX + 5, avatarY, ANCHOR_ROW);
                    }
                    avatarY += ROW_HEIGHT;
                }
            }
#ifdef MEDECOMP_PORT
            int focus = medecomp_challenge_focus_row();
            if (focus >= 0) {
                g->setColor(0xe0, 0x20, 0x20, 0x60);
                g->fillRect(originX, originY + focus * ROW_HEIGHT, m_width, ROW_HEIGHT);
            }
#endif
        }
    }
    if (m_selected != -1 && !m_effectPlayed) {
        int x = m_x + originX + (m_width >> 1);
        int y = originY + m_selected * ROW_HEIGHT + m_y + 0x1c;
        engine->getWindowStore().getButtonEffect()->play(x, y);
        m_effectPlayed = true;
    }
}

bool ChallengeUserList::pointerReleased(int x, int y, int id) {
    (void)x;
    (void)id;
    AppEngine* engine = AppEngine::getCanvas();
    int row = y / ROW_HEIGHT;
    m_selected = row;
    if (static_cast<unsigned>(row) < m_userIds.size()) {
        engine->getSceneGame()->m_challengeUserId = m_userIds[static_cast<unsigned>(row)];
        m_owner->close(RESULT_CHOSEN);
        return true;
    }
    return false;
}

ChallengeUserList::ChallengeUserList(ChallengeUserWindow& owner,
                                     midp::SafeReferenceCountedPointer<Mayhem::GetUserListRequest> request,
                                     int width)
    : m_owner(&owner),
      m_listImage(NULL),
      m_selected(-1),
      m_effectPlayed(false) {
    TextManager* text = AppEngine::getCanvas()->getTextManager();
    setWidth(width);
    int count = asUserList(request)->Size();
    int validCount = 0;
    if (count >= 1) {
        for (int i = 0; i < count; i++) {
            Mayhem::User* user = asUserList(request)->GetMayhemUser(i);
            if (user->IsValid()) {
                validCount += 1;
            }
        }
        if (validCount >= 1) {
            int imageH = validCount * ROW_HEIGHT;
            midp::ReferenceCountedPointer<midp::Image> image(midp::Image::createImage(m_width, imageH));
            midp::ReferenceCountedPointer<midp::Graphics> graphics(image->getGraphics());
            graphics->setClip(0, 0, m_width, imageH);
            m_avatars.reserve(static_cast<unsigned>(count));
            if (count >= 1) {
                int rowY = 0;
                bool colored = true;
                for (int i = 0; i < count; i++) {
                    (void)AppEngine::getCanvas();
                    FBConnection* connection = 0;
                    const FBFriend& facebook = connection->GetFriend(i);
                    Mayhem::User* user = asUserList(request)->GetMayhemUser(i);
                    if (!user->IsValid()) {
                        continue;
                    }
                    midp::ReferenceCountedPointer<midp::String> label;
                    midp::ReferenceCountedPointer<midp::String> url(new midp::String(facebook.GetPicURL().c_str()));
                    midp::WebImage* created = NULL;
                    if (url->length() >= 1) {
                        created = new midp::WebImage(url);
                    }
                    {
                        midp::ReferenceCountedPointer<midp::WebImage> avatar(created);
                        m_avatars.push_back(avatar);
                    }
                    midp::StringBuffer* buffer = text->clearStringBuffer();
#ifdef MEDECOMP_PORT
                    if (facebook.GetName().empty()) {
                        text->appendStringToBuffer(user->GetDisplayName().c_str());
                    } else
#endif
                    {
                        text->appendStringToBuffer(facebook.GetName().c_str());
                        text->appendStringToBuffer(" (");
                        text->appendStringToBuffer(user->GetDisplayName().c_str());
                        text->appendStringToBuffer(")");
                    }
                    label = buffer->toString();
                    int color;
                    if (colored) {
                        color = COLOR_STRIPE;
                    } else {
                        graphics->setColor(255, 255, 255, 95);
                        color = COLOR_PLAIN;
                    }
                    graphics->setColor((color >> 16) & 0xff, (color >> 8) & 0xff, 255, 0x7f);
                    graphics->fillRect(0, rowY, m_width, rowY + ROW_HEIGHT);
                    text->drawString(graphics, label, FONT_NAME, 0x3c, rowY + 0x1c, ANCHOR_NAME);
                    m_userIds.push_back(user->GetMayhemID());
                    rowY += ROW_HEIGHT;
                    colored = !colored;
                }
            }
            m_listImage = image;
        }
    }
    setHeight(validCount * ROW_HEIGHT);
}

ChallengeUserList::~ChallengeUserList() {}

}
