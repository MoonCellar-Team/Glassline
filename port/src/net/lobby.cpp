#include <ctype.h>

#include <array>
#include <map>
#include <string>

#include "game/AppEngine.h"
#include "game/Level.h"
#include "game/LevelData.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"
#include "game/UI/MajorButton.h"
#include "game/UI/TitledWindow.h"
#include "game/WrappedString.h"
#include "port/net.h"
#include "port/online.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"
#include "text.h"

extern "C" const char* medecomp_line(const void* owner, int limit, int* key);
extern "C" void medecomp_line_close(const void* owner);

namespace port {
namespace net {

namespace {

const int TEXT_TITLE = 0x930;
const int TEXT_BACK = 0x82f;
const int TEXT_OK = 0x82e;
const int TEXT_CREATE = 0x980;
const int TEXT_JOIN = 0x981;
const int TEXT_CHOOSE = 0x982;
const int TEXT_SPLIT = 0x9bc;
const int TEXT_INTRO = 0x983;
const int TEXT_CODE = 0x984;
const int TEXT_GIVE_CODE = 0x985;
const int TEXT_ENTER_CODE = 0x986;
const int TEXT_CONNECTING = 0x987;
const int TEXT_OPPONENT = 0x988;
const int TEXT_CHOOSE_HINT = 0x989;
const int TEXT_HOST_CHOOSING = 0x98a;
const int TEXT_CONNECT_FAILED = 0x98b;
const int TEXT_NO_LIBRARY = 0x993;
const int TEXT_NO_P2P = 0x994;
const int TEXT_SETUP = 0x995;
const int TEXT_SERVER_UNREACHABLE = 0x996;
const int TEXT_SERVER_LOST = 0x997;
const int TEXT_NO_SUCH_GAME = 0x998;
const int TEXT_GAME_FULL = 0x999;
const int TEXT_HOST_LEFT = 0x99a;
const int TEXT_NO_ROUTE = 0x99b;
const int TEXT_PEER_LEFT = 0x99c;
const int TEXT_VERSION = 0x9ba;
const int TEXT_MODE_RACE = 0x941, TEXT_MODE_TIMED = 0x942;
const int TEXT_TIME_2MIN = 0x94e, TEXT_TIME_3MIN = 0x94f, TEXT_TIME_5MIN = 0x950;
const int TEXT_GETTING_MAP = 0x9c9;
const int TEXT_MAP_FAILED = 0x9ca;
const int TEXT_SERVER_PING = 0x9dd;
const int TEXT_PEER_PING = 0x9de;
const int PING_ABOVE_FRAME_BOTTOM = 16;
const int ANCHOR_BOTTOM_CENTER = 0x22;

const int FONT_BODY = 2;
const int FONT_CODE = 0x1c;
const int ANCHOR_TOP_CENTER = 10;
const int OFFSCREEN = -3000;
const int FRAME_X = 70, FRAME_Y = 190, FRAME_BOTTOM = 160;
const int TEXT_INSET = 40;
const int BUTTON_GAP = 16;
const int BACK_RIGHT = 69, BACK_BOTTOM = 36;
const int CODE_LIMIT = 8;
const int KEY_RETURN = 13, KEY_ESCAPE = 27;
const int CARET_BLINK_MS = 500;

std::string hostChoiceText() {
    HostBrowsing b;
    LevelData* levels = AppEngine::getLevelData();
    if (!raceHostBrowsing(&b) || b.level >= levels->getLevelNum()) return std::string();
    std::string text = gameText(b.race ? TEXT_MODE_RACE : TEXT_MODE_TIMED) + " — " +
                       (b.playerLevel.empty() ? gameText(levels->getLevel(b.level)->getName()) : b.playerLevel);
    if (!b.race && b.timeLimit > 0)
        text += " — " + gameText(b.timeLimit <= 120000 ? TEXT_TIME_2MIN : b.timeLimit >= 300000 ? TEXT_TIME_5MIN : TEXT_TIME_3MIN);
    return text;
}

std::string mapTransferText() {
    int percent = -1;
    bool failed = false;
    if (!raceGettingLevel(&percent, &failed)) return std::string();
    if (failed) return gameText(TEXT_MAP_FAILED);
    return gameText(TEXT_GETTING_MAP) + (percent >= 0 ? " " + std::to_string(percent) + "%" : std::string());
}

std::string pingText(const SessionInfo& info) {
    std::string text;
    if (info.serverPing >= 0) text = replaced(gameText(TEXT_SERVER_PING), "{ms}", std::to_string(info.serverPing));
    if (info.ping >= 0) {
        std::string peer = replaced(replaced(gameText(TEXT_PEER_PING), "{name}", info.peerName.empty() ? "?" : info.peerName),
                                    "{ms}", std::to_string(info.ping));
        text += (text.empty() ? "" : "     ") + peer;
    }
    return text;
}

int errorText(SessionError error) {
    switch (error) {
    case SessionError::NoLibrary: return TEXT_NO_LIBRARY;
    case SessionError::NoP2P: return TEXT_NO_P2P;
    case SessionError::Setup: return TEXT_SETUP;
    case SessionError::ServerUnreachable: return TEXT_SERVER_UNREACHABLE;
    case SessionError::ServerLost: return TEXT_SERVER_LOST;
    case SessionError::NoSuchGame: return TEXT_NO_SUCH_GAME;
    case SessionError::GameFull: return TEXT_GAME_FULL;
    case SessionError::HostLeft: return TEXT_HOST_LEFT;
    case SessionError::NoRoute: return TEXT_NO_ROUTE;
    case SessionError::PeerLeft: return TEXT_PEER_LEFT;
    case SessionError::VersionMismatch: return TEXT_VERSION;
    case SessionError::None:
    case SessionError::ConnectFailed: break;
    }
    return TEXT_CONNECT_FAILED;
}

bool s_splitScreen = false;

class LobbyWindow : public UI::TitledWindow {
public:
    LobbyWindow()
        : TitledWindow(TEXT_TITLE, -1),
          m_create(TEXT_CREATE),
          m_split(TEXT_SPLIT),
          m_join(TEXT_JOIN),
          m_choose(TEXT_CHOOSE),
          m_ok(TEXT_OK),
          m_back(TEXT_BACK) {
        m_background.setPosition(FRAME_X, FRAME_Y);
        m_background.setDimensions(m_width - 2 * FRAME_X, m_height - FRAME_Y - FRAME_BOTTOM);
        m_backButton.setPosition(OFFSCREEN, OFFSCREEN);
        m_back.setPosition(m_width - BACK_RIGHT - m_back.getWidth(), m_height - BACK_BOTTOM - m_back.getHeight());
        s_splitScreen = false;
        refresh(true);
    }

    ~LobbyWindow() override {
        medecomp_line_close(this);
        for (UI::MajorButton* b : buttons()) place(b, false);
    }

    void update(int dt) override {
        if (m_back.isActivated()) {
            m_back.reset();
            back();
            return;
        }
        if (m_entering) {
            enterCode(dt);
            return;
        }
        if (m_create.isActivated()) {
            m_create.reset();
            sessionHost();
        }
        if (m_split.isActivated()) {
            m_split.reset();
            sessionClose();
            s_splitScreen = true;
            close(static_cast<UI::WindowResult>(kLobbyChooseLevel));
            return;
        }
        if (m_join.isActivated()) {
            m_join.reset();
            m_entering = true;
            m_typed.clear();
            m_caretMs = 0;
            refresh(true);
            return;
        }
        if (m_choose.isActivated()) {
            m_choose.reset();
            s_splitScreen = false;
            close(static_cast<UI::WindowResult>(kLobbyChooseLevel));
            return;
        }
        SessionInfo info = sessionInfo();
        if (info.state == SessionState::Connected && info.host) raceBrowsing(false, nullptr, 0, false, nullptr);
        RaceChoice choice;
        if (info.state == SessionState::Connected && !info.host && raceChoiceToJoin(&choice)) {
            raceJoined();
            raceApplyChoice(choice);
            AppEngine::getCanvas()->getSceneMenu()->stateTransitionFade(SceneMenu::STATE_TRANSITION_TO_GAME);
            close(UI::RESULT_NONE);
            return;
        }
        refresh(false);
    }

    void render(midp::Graphics* g, int originY, int originX) override {
        TitledWindow::render(g, originY, originX);
        TextManager* text = AppEngine::getCanvas()->getTextManager();
        int centerX = m_width / 2;
        int y = FRAME_Y + TEXT_INSET;
        if (m_codeLabel) {
            text->drawString(g, m_codeLabel, FONT_BODY, centerX, y, ANCHOR_TOP_CENTER);
            y += text->getLineHeight(FONT_BODY) + 8;
            if (m_code) text->drawString(g, m_code, FONT_CODE, centerX, y, ANCHOR_TOP_CENTER);
            if (m_entering && (m_caretMs / CARET_BLINK_MS) % 2 == 0) {
                int width = m_code ? text->getStringWidth(m_code, FONT_CODE) : 0;
                g->setColor(0);
                g->fillRect(centerX + width / 2 + 3, y, 3, text->getLineHeight(FONT_CODE));
            }
            y += text->getLineHeight(FONT_CODE) + 24;
        }
        m_wrapped.draw(g, centerX, y, ANCHOR_TOP_CENTER);
        std::string ping = m_entering ? std::string() : pingText(sessionInfo());
        if (ping != m_pingShown) {
            m_pingShown = ping;
            m_ping = ping.empty() ? nullptr : fromUtf8(ping);
        }
        if (m_ping)
            text->drawString(g, m_ping, FONT_BODY, centerX, m_height - FRAME_BOTTOM - PING_ABOVE_FRAME_BOTTOM,
                             ANCHOR_BOTTOM_CENTER);
        Window::render(g, originY, originX);
    }

    bool pointerPressed(int x, int y, int id) override {
        UI::MajorButton* b = hit(x, y);
        if (b && !b->m_enabled && m_netReason >= 0) {
            m_message = fromUtf8(gameText(m_netReason));
            m_wrapped.wrapString(m_message, FONT_BODY, m_background.getWidth() - 2 * TEXT_INSET);
            return true;
        }
        if (b) b->pointerPressed(x, y, id);
        return b != nullptr;
    }

    bool pointerReleased(int x, int y, int id) override {
        UI::MajorButton* b = hit(x, y);
        if (b && !b->isPressed()) b = nullptr;
        if (b) b->pointerReleased(x, y, id);
        for (UI::MajorButton* other : buttons())
            if (other != b && other->isPressed()) other->unpress();
        return b != nullptr;
    }

    bool pointerDragged(int x, int y, int id) override {
        (void)id;
        for (UI::MajorButton* b : buttons())
            if (b->isPressed() && !b->contains(x, y)) b->unpress();
        return false;
    }

private:
    std::array<UI::MajorButton*, 6> buttons() { return {&m_create, &m_split, &m_join, &m_choose, &m_ok, &m_back}; }

    UI::MajorButton* hit(int x, int y) {
        for (UI::MajorButton* b : buttons())
            if (m_shown[b] && b->contains(x, y)) return b;
        return nullptr;
    }

    void back() {
        if (m_entering) {
            stopEntering();
            refresh(true);
            return;
        }
        if (sessionInfo().state == SessionState::Idle) {
            close(UI::RESULT_CANCEL);
            return;
        }
        sessionClose();
        refresh(true);
    }

    void stopEntering() {
        m_entering = false;
        medecomp_line_close(this);
    }

    void enterCode(int dt) {
        m_caretMs += dt;
        int key = 0;
        std::string clean;
        for (const char* c = medecomp_line(this, CODE_LIMIT, &key); *c; c++)
            if (isalnum(static_cast<unsigned char>(*c))) clean += char(toupper(static_cast<unsigned char>(*c)));
        if (clean != m_typed) {
            m_typed = clean;
            m_caretMs = 0;
            refresh(true);
        }
        if (m_ok.isActivated()) {
            m_ok.reset();
            key = KEY_RETURN;
        }
        if (key == KEY_ESCAPE) {
            back();
        } else if (key == KEY_RETURN && !m_typed.empty()) {
            stopEntering();
            sessionJoin(m_typed);
            refresh(true);
        }
    }

    int netReason() {
        if (!port::net::available()) return port::net::libraryFound() ? TEXT_SETUP : TEXT_NO_LIBRARY;
        if (!port::online::available()) return TEXT_SERVER_UNREACHABLE;
        return -1;
    }

    void showButtons(bool create, bool join, bool choose, bool ok = false) {
        if (create || join) {
            m_netReason = netReason();
            m_create.setEnabled(m_netReason < 0);
            m_join.setEnabled(m_netReason < 0);
        }
        place(&m_create, create);
        place(&m_split, create);
        place(&m_join, join);
        place(&m_choose, choose);
        place(&m_ok, ok);
        place(&m_back, true);
        int x = m_back.getX();
        for (UI::MajorButton* b : {&m_ok, &m_choose, &m_join, &m_create, &m_split}) {
            if (!m_shown[b]) continue;
            x -= BUTTON_GAP + b->getWidth();
            b->setPosition(x, m_back.getY());
        }
    }

    void place(UI::MajorButton* b, bool show) {
        if (show == m_shown[b]) return;
        if (show)
            addElement(b);
        else
            removeElement(b);
        m_shown[b] = show;
    }

    void refresh(bool force) {
        SessionInfo info = sessionInfo();
        std::string choice, transfer;
        if (info.state == SessionState::Connected && !info.host) {
            choice = hostChoiceText();
            transfer = mapTransferText();
            if (!transfer.empty()) choice += "\n" + transfer;
        }
        if (!force && info.state == m_state && info.code == m_lastCode && info.peerName == m_lastPeer &&
            info.error == m_lastError && choice == m_lastChoice)
            return;
        if (m_entering) {
            m_codeLabel = fromUtf8(gameText(TEXT_ENTER_CODE));
            m_code = m_typed.empty() ? nullptr : fromUtf8(m_typed);
            m_message = fromUtf8("");
            m_wrapped.wrapString(m_message, FONT_BODY, m_background.getWidth() - 2 * TEXT_INSET);
            showButtons(false, false, false, !m_typed.empty());
            return;
        }
        m_state = info.state;
        m_lastCode = info.code;
        m_lastPeer = info.peerName;
        m_lastError = info.error;
        m_lastChoice = choice;
        m_codeLabel = nullptr;
        m_code = nullptr;
        std::string message;
        switch (info.state) {
        case SessionState::Idle:
            message = gameText(TEXT_INTRO);
            showButtons(true, true, false);
            break;
        case SessionState::Opening:
        case SessionState::WaitingGuest:
            if (info.host && !info.code.empty()) {
                m_codeLabel = fromUtf8(gameText(TEXT_CODE));
                m_code = fromUtf8(info.code);
                message = gameText(TEXT_GIVE_CODE);
            } else {
                message = gameText(TEXT_CONNECTING);
            }
            showButtons(false, false, false);
            break;
        case SessionState::Connecting:
            message = gameText(TEXT_CONNECTING);
            showButtons(false, false, false);
            break;
        case SessionState::Connected:
            message = replaced(gameText(TEXT_OPPONENT), "{name}", info.peerName) + "\n" +
                      gameText(info.host ? TEXT_CHOOSE_HINT : TEXT_HOST_CHOOSING);
            if (!choice.empty()) message += "\n" + choice;
            showButtons(false, false, info.host);
            break;
        case SessionState::Failed:
            message = gameText(errorText(info.error));
            showButtons(true, true, false);
            break;
        }
        m_message = fromUtf8(message);
        m_wrapped.wrapString(m_message, FONT_BODY, m_background.getWidth() - 2 * TEXT_INSET);
    }

    UI::MajorButton m_create, m_split, m_join, m_choose, m_ok, m_back;
    std::map<UI::MajorButton*, bool> m_shown;
    bool m_entering = false;
    int m_netReason = -1;
    std::string m_typed;
    int m_caretMs = 0;
    SessionState m_state = SessionState::Idle;
    std::string m_lastCode, m_lastPeer, m_lastChoice, m_pingShown;
    SessionError m_lastError = SessionError::None;
    midp::ReferenceCountedPointer<midp::String> m_codeLabel, m_code, m_message, m_ping;
    WrappedString m_wrapped;
};

}

bool splitScreenChosen() { return s_splitScreen; }

UI::Window* createLobbyWindow() { return new LobbyWindow(); }

}
}
