#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <string>
#include <thread>

#include "../ea/json.h"
#include "port/net.h"

namespace port {
namespace net {

namespace {

using Clock = std::chrono::steady_clock;

const int kConnectTimeoutMs = 25000;

SessionState s_state = SessionState::Idle;
bool s_host;
std::string s_code, s_peerName, s_joinCode, s_peerIdentity;
int s_peerVersion = -1;
SessionError s_error = SessionError::None;
Peer s_peer;
bool s_p2pStarted;
Clock::time_point s_since;
std::atomic<int> s_opened(0);
std::string s_openError;
std::deque<Received> s_gameEvents;
std::deque<std::vector<uint8_t>> s_lobby;
std::deque<Received> s_early;

void deliver(const Received& r) {
    if (!r.data.empty() && r.data[0] >= kLobbyMessage)
        s_lobby.push_back(r.data);
    else
        s_gameEvents.push_back(r);
}

void fail(SessionError kind, const std::string& detail) {
    s_state = SessionState::Failed;
    s_error = kind;
    signalClose();
    if (s_peer) disconnect(s_peer);
    s_peer = 0;
    log("%s", detail.c_str());
}

SessionError roomError(const std::string& message) {
    if (message.find("No game with this code") != std::string::npos) return SessionError::NoSuchGame;
    if (message.find("two players") != std::string::npos) return SessionError::GameFull;
    return SessionError::ConnectFailed;
}

void failUnavailable() {
    std::string why;
    if (available(&why))
        fail(SessionError::NoP2P, "в библиотеке сети нет P2P (ICE)");
    else
        fail(libraryFound() ? SessionError::Setup : SessionError::NoLibrary, why);
}

void openSignal() {
    s_opened = 0;
    s_openError.clear();
    std::thread([]() {
        std::string error;
        bool ok = signalOpen(&error);
        s_openError = error;
        s_opened = ok ? 1 : -1;
    }).detach();
}

void startP2P() {
    if (s_p2pStarted || s_peerIdentity.empty()) return;
    s_p2pStarted = true;
    if (s_host) return;
    std::string error;
    s_peer = connectP2P(s_peerIdentity, &error);
    if (!s_peer) fail(SessionError::ConnectFailed, error);
}

void onSignal(const SignalReceived& s) {
    if (!s.turn.servers.empty()) setTurn(s.turn.servers, s.turn.user, s.turn.pass);
    switch (s.event) {
    case SignalEvent::Hosted:
        s_code = s.code;
        s_state = SessionState::WaitingGuest;
        log("комната %s", s_code.c_str());
        break;
    case SignalEvent::Joined:
    case SignalEvent::Peer:
        s_peerName = s.peer.name;
        log("в комнате %s, ищу путь (ICE)", s_peerName.c_str());
        s_state = SessionState::Connecting;
        s_since = Clock::now();
        signalSend("ver:" + std::to_string(kNetVersion));
        signalSend("id:" + identity());
        break;
    case SignalEvent::Message:
        if (s_state == SessionState::Failed) break;
        if (s.text.compare(0, 4, "ver:") == 0) {
            s_peerVersion = atoi(s.text.c_str() + 4);
            if (s_peerVersion != kNetVersion)
                fail(SessionError::VersionMismatch, "у соперника версия протокола " + std::to_string(s_peerVersion) +
                                                        ", у меня " + std::to_string(kNetVersion));
        } else if (s.text.compare(0, 3, "id:") == 0) {
            if (s_peerVersion < 0) {
                fail(SessionError::VersionMismatch, "соперник не прислал версию протокола (старая сборка)");
                break;
            }
            s_peerIdentity = s.text.substr(3);
            startP2P();
        } else if (s.text.compare(0, 4, "ice:") == 0) {
            std::string blob;
            if (port::online::base64Decode(s.text.substr(4), &blob)) receivedSignal(blob);
        }
        break;
    case SignalEvent::Left:
        if (s_state != SessionState::Connected) {
            s_state = SessionState::WaitingGuest;
            s_peerIdentity.clear();
            s_peerVersion = -1;
            s_p2pStarted = false;
        }
        break;
    case SignalEvent::Closed:
        if (s_state != SessionState::Connected) {
            if (s.text.empty())
                fail(SessionError::HostLeft, "хост закрыл комнату");
            else
                fail(SessionError::ServerLost, s.text);
        }
        break;
    case SignalEvent::Error:
        if (s_state != SessionState::Connected) fail(roomError(s.text), "сигнальный сервер: " + s.text);
        break;
    }
}

}

const char* sessionErrorName(SessionError error) {
    switch (error) {
    case SessionError::None: return "нет ошибки";
    case SessionError::NoLibrary: return "нет библиотеки сети";
    case SessionError::NoP2P: return "в библиотеке сети нет P2P";
    case SessionError::Setup: return "сеть не запустилась";
    case SessionError::ServerUnreachable: return "сигнальный сервер не отвечает";
    case SessionError::ServerLost: return "связь с сигнальным сервером оборвалась";
    case SessionError::NoSuchGame: return "нет комнаты с этим кодом";
    case SessionError::GameFull: return "в комнате уже двое";
    case SessionError::HostLeft: return "хост закрыл комнату";
    case SessionError::NoRoute: return "пути между компьютерами не нашлось";
    case SessionError::ConnectFailed: return "соединиться не удалось";
    case SessionError::PeerLeft: return "соперник отключился";
    case SessionError::VersionMismatch: return "у соперника другая версия игры";
    }
    return "?";
}

bool sessionHost() {
    sessionClose();
    if (!p2pAvailable()) {
        failUnavailable();
        return false;
    }
    std::string error;
    if (!listenP2P(&error)) {
        fail(SessionError::Setup, error);
        return false;
    }
    s_host = true;
    s_state = SessionState::Opening;
    openSignal();
    return true;
}

bool sessionJoin(const std::string& code) {
    sessionClose();
    if (!p2pAvailable()) {
        failUnavailable();
        return false;
    }
    s_host = false;
    s_joinCode = code;
    s_state = SessionState::Opening;
    openSignal();
    return true;
}

void sessionUseConnection(Peer peer, bool host) {
    s_host = host;
    s_peer = peer;
    s_state = peer ? SessionState::Connecting : SessionState::WaitingGuest;
    s_since = Clock::now();
}

void sessionClose() {
    if (s_peer) disconnect(s_peer);
    signalClose();
    s_state = SessionState::Idle;
    s_peer = 0;
    s_code.clear();
    s_peerName.clear();
    s_error = SessionError::None;
    s_peerIdentity.clear();
    s_peerVersion = -1;
    s_p2pStarted = false;
    s_gameEvents.clear();
    s_lobby.clear();
    s_early.clear();
}

SessionInfo sessionInfo() {
    SessionInfo info;
    info.state = s_state;
    info.host = s_host;
    info.code = s_code;
    info.peerName = s_peerName;
    info.error = s_error;
    info.ping = s_peer && s_state == SessionState::Connected ? ping(s_peer) : -1;
    return info;
}

Peer sessionPeer() { return s_state == SessionState::Connected ? s_peer : 0; }

void sessionFrame() {
    if (s_state == SessionState::Opening && s_opened != 0) {
        if (s_opened < 0) {
            fail(SessionError::ServerUnreachable, "сигнальный сервер: " + s_openError);
        } else if (s_host) {
            signalHost(0);
            s_state = SessionState::WaitingGuest;
        } else {
            signalJoin(s_joinCode, 0);
            s_state = SessionState::Connecting;
            s_since = Clock::now();
        }
    }
    if (s_state != SessionState::Opening) {
        SignalReceived s;
        while (signalPoll(&s)) onSignal(s);
    }
    std::string blob;
    while (takeSignal(&blob)) signalSend("ice:" + port::online::base64Encode(blob.data(), blob.size()));

    Received r;
    while (poll(&r)) {
        if (r.event == Event::Connected) {
            if (!s_peer || s_peer == r.peer) {
                s_peer = r.peer;
                s_state = SessionState::Connected;
                s_gameEvents.push_back(r);
                for (const Received& early : s_early)
                    if (early.peer == r.peer) deliver(early);
                s_early.clear();
            } else {
                disconnect(r.peer);
            }
        } else if (r.event == Event::Disconnected) {
            if (r.peer != s_peer) continue;
            bool wasConnected = s_state == SessionState::Connected;
            s_gameEvents.push_back(r);
            s_peer = 0;
            if (wasConnected) {
                s_state = SessionState::Failed;
                s_error = SessionError::PeerLeft;
            } else if (s_state != SessionState::Idle) {
                fail(SessionError::ConnectFailed, "соединиться не удалось: " + r.text);
            }
        } else if (r.peer == s_peer && s_state == SessionState::Connected && !r.data.empty()) {
            deliver(r);
        } else if (s_state != SessionState::Connected && !r.data.empty() && s_early.size() < 256) {
            s_early.push_back(r);
        }
    }
    if (s_state == SessionState::Connecting &&
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - s_since).count() > kConnectTimeoutMs)
        fail(SessionError::NoRoute, "соединиться не удалось: пути между компьютерами не нашлось");
}

bool sessionPoll(Received* out) {
    if (s_gameEvents.empty()) return false;
    *out = std::move(s_gameEvents.front());
    s_gameEvents.pop_front();
    return true;
}

bool sessionTakeLobby(std::vector<uint8_t>* message) {
    if (s_lobby.empty()) return false;
    *message = std::move(s_lobby.front());
    s_lobby.pop_front();
    return true;
}

bool sessionSend(const void* data, size_t size, bool reliable) {
    Peer peer = sessionPeer();
    return peer && send(peer, data, size, reliable);
}

}
}
