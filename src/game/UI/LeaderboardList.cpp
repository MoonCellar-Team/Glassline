#include "game/UI/LeaderboardList.h"

#include "ea_online/Mayhem/Request.h"
#include "ea_online/Mayhem/Stat.h"
#include "ea_online/Mayhem/User.h"
#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"
#include "game/UI/ButtonEffect.h"
#include "game/UI/LeaderboardWindow.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/Image.h"
#include "runtime/midp/OutputStream.h"
#include "runtime/midp/String.h"
#include "runtime/midp/StringBuffer.h"
#include "sdk/CoreFoundation.h"

extern "C" CFStringRef CFStringCreateWithCString(const void* alloc, const char* cStr, CFStringEncoding encoding);

namespace UI {

static const int ROW_HEIGHT = 35;
static const int ROW_TEXT_DY = 17;
static const int FONT_ROW = 2;
static const int ANCHOR_ROW = 0x11;
static const int ANCHOR_IMAGE = 9;
static const int RANK_X = 17;
static const int NAME_X = 90;
static const int TIME_X = 720;
static const int NAME_TAIL = 25;
static const char* const GHOST_FILE = "ChallengeGhost";
static const WindowResult RESULT_PICK = (WindowResult)3;

const int LeaderboardList::MAX_ENTRIES = 29;

void LeaderboardList::render(midp::Graphics* g, int originY, int originX) {
    if (m_image == NULL) {
        cache();
    }
    if (m_image != NULL) {
        g->drawImage(m_image, originX, originY, ANCHOR_IMAGE);
    }
    if (m_selected == -1) {
        return;
    }
    ButtonEffect* effect = AppEngine::getCanvas()->getWindowStore().getButtonEffect();
    if (!m_effectPlayed) {
        int x = m_x + originX + (m_width >> 1);
        int y = m_selected * ROW_HEIGHT + originY + m_y + ROW_TEXT_DY;
        effect->play(x, y);
        m_effectPlayed = true;
        return;
    }
    if (effect->isAnimating()) {
        return;
    }
    if (m_statRequest->GetResult() != Mayhem::Request::RESULT_SUCCESS) {
        return;
    }
    void* data = NULL;
    int length = m_statRequest->GetDecodedData(&data);
    if (length > 0) {
        midp::String* name = new midp::String(GHOST_FILE);
        midp::ReferenceCountedPointer<midp::OutputStream> out(midp::OutputStream::getResourceAsStream(name));
        if (out != NULL) {
            out->write(data, length);
            out->close();
        }
    }
    AppEngine* canvas = AppEngine::getCanvas();
    AppEngine::getLevelData()->setCurrentLevelByIndex(LevelData::GAME_MODE_CHALLENGE, m_levelIndex);
    canvas->getSceneMenu()->stateTransition(SceneMenu::STATE_CHALLENGE_CONFIRM);
    canvas->setChallengeTime(canvas->mayhemScoreToTime(m_statRequest->GetValue(), m_levelIndex));
    m_window->close(RESULT_PICK);
}

bool LeaderboardList::pointerReleased(int, int y, int) {
    if (m_selected != -1) {
        return false;
    }
    m_selected = y / ROW_HEIGHT;
    midp::SafeReferenceCountedPointer<Mayhem::GetStatRequest> request(
        new Mayhem::GetStatRequest(m_statIds[m_selected], true));
    m_statRequest = request;
    return true;
}

int LeaderboardList::cache() {
    AppEngine* canvas = AppEngine::getCanvas();
    TextManager* text = canvas->getTextManager();
    midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> board(m_request);
    int count = board->Size();
    if (count > MAX_ENTRIES) {
        count = MAX_ENTRIES;
    }
    if (count < 1) {
        return count;
    }
    int imageHeight = count * ROW_HEIGHT;
    midp::ReferenceCountedPointer<midp::Image> image(midp::Image::createImage(m_width, imageHeight));
    midp::ReferenceCountedPointer<midp::Graphics> graphics(image->getGraphics());
    graphics->setClip(0, 0, m_width, imageHeight);

    bool stripe = true;
    for (int index = 0; index < count; ++index) {
        int y = index * ROW_HEIGHT;
        int textY = y + ROW_TEXT_DY;
        Mayhem::User* user = board->GetUser(index);
        Mayhem::Stat* stat = board->GetStat(index);

        midp::ReferenceCountedPointer<midp::String> rank;
        midp::ReferenceCountedPointer<midp::String> name;
        midp::ReferenceCountedPointer<midp::String> time;
        midp::StringBuffer* buffer = text->clearStringBuffer();
        text->appendIntToBuffer(buffer, index + 1);
        rank = buffer->toString();

        buffer = text->clearStringBuffer();
        const std::string& display = user->GetDisplayName();
        if (display.length() == 0) {
            unsigned length = user->GetMayhemID().length();
            std::string tail = user->GetMayhemID().substr(length - NAME_TAIL, NAME_TAIL);
            text->appendStringToBuffer(tail.c_str());
        } else {
            CFStringRef cf = CFStringCreateWithCString(NULL, display.c_str(), kCFStringEncodingUTF8);
            midp::ReferenceCountedPointer<midp::String> label(new midp::String(cf));
            text->appendStringToBuffer(label);
        }
        name = buffer->toString();

        buffer = text->clearStringBuffer();
        int millis = canvas->mayhemScoreToTime(stat->GetValue(), m_levelIndex);
        text->appendMillisTimeToBuffer(buffer, millis, 2);
        time = buffer->toString();

        int color = 0x7fffffff;
        if (stripe) {
            color = 0x7fd4e8ff;
        } else {
            graphics->setColor(255, 255, 255, 0x5f);
        }
        graphics->setColor((color >> 16) & 0xff, (color >> 8) & 0xff, 0xff, 0x7f);
        graphics->fillRect(0, y, m_width, y + ROW_HEIGHT);
        text->drawString(graphics, rank, FONT_ROW, RANK_X, textY, ANCHOR_ROW);
        text->drawString(graphics, name, FONT_ROW, NAME_X, textY, ANCHOR_ROW);
        text->drawString(graphics, time, FONT_ROW, TIME_X, textY, ANCHOR_ROW);
        m_statIds.push_back(stat->GetStatID());
        stripe = !stripe;
    }
    m_image = image;
    return count;
}

LeaderboardList::LeaderboardList(LeaderboardWindow& window, int levelIndex,
                                  midp::SafeReferenceCountedPointer<Mayhem::GetLeaderboardRequest> request,
                                  int width)
    : m_window(&window),
      m_image(NULL),
      m_levelIndex(levelIndex),
      m_request(request),
      m_selected(-1),
      m_effectPlayed(false),
      m_statIds(),
      m_statRequest(NULL) {
    setWidth(width);
    setHeight(cache() * ROW_HEIGHT);
}

void LeaderboardList::uncache() {
    m_image = NULL;
}

LeaderboardList::~LeaderboardList() {}

}
