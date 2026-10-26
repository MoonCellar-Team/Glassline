#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
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
const size_t kRelayPart = 3000;
const int kRelayPingMs = 2000;

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
bool s_relayMine;
bool s_relay;
std::vector<uint8_t> s_relayPartial;
long long s_relayPingAt;
long long s_relayPingNext;
int s_relayRtt = -1;

long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
}

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

bool relayOnly() {
    const char* forced = getenv("MEDECOMP_NET_RELAY");
    return (forced && *forced && strcmp(forced, "0") != 0) || !p2pAvailable();
}

void startRelay() {
    if (s_relay) return;
    s_relay = true;
    s_peer = kRelayPeer;
    s_state = SessionState::Connected;
    s_relayPingNext = 0;
    Received r;
    r.event = Event::Connected;
    r.peer = kRelayPeer;
    r.text = "ретранслятор";
    s_gameEvents.push_back(r);
    log("соединение через ретранслятор комнаты (%s)", s_relayMine ? "у себя P2P нет" : "у соперника P2P нет");
}

bool relayReceived(const std::string& text) {
    if (text.compare(0, 2, "r:") == 0 || text.compare(0, 2, "R:") == 0) {
        std::string bytes;
        if (!port::online::base64Decode(text.substr(2), &bytes)) {
            log("ретранслятор: кусок не разобрать (%zu знаков)", text.size());
            return true;
        }
        s_relayPartial.insert(s_relayPartial.end(), bytes.begin(), bytes.end());
        if (text[0] == 'R') return true;
        Received r;
        r.event = Event::Message;
        r.peer = kRelayPeer;
        r.data.swap(s_relayPartial);
        if (s_state == SessionState::Connected)
            deliver(r);
        else if (s_early.size() < 256)
            s_early.push_back(r);
        return true;
    }
    if (text.compare(0, 3, "rp:") == 0) {
        signalSend("ro:" + text.substr(3));
        return true;
    }
    if (text.compare(0, 3, "ro:") == 0) {
        if (s_relayPingAt && atoll(text.c_str() + 3) == s_relayPingAt) {
            s_relayRtt = int(nowMs() - s_relayPingAt);
            s_relayPingAt = 0;
        }
        return true;
    }
    return false;
}

void relayLost(SessionError kind, const std::string& detail) {
    Received r;
    r.event = Event::Disconnected;
    r.peer = kRelayPeer;
    r.text = detail;
    s_gameEvents.push_back(r);
    s_peer = 0;
    s_relay = false;
    s_state = SessionState::Failed;
    s_error = kind;
    log("ретранслятор: %s", detail.c_str());
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
        signalSend(s_relayMine ? std::string("id:relay") : "id:" + identity());
        break;
    case SignalEvent::Message:
        if (s_state == SessionState::Failed) break;
        if (relayReceived(s.text)) break;
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
            if (s_relayMine || s_peerIdentity == "relay")
                startRelay();
            else
                startP2P();
        } else if (s.text.compare(0, 4, "ice:") == 0) {
            std::string blob;
            if (port::online::base64Decode(s.text.substr(4), &blob)) receivedSignal(blob);
        }
        break;
    case SignalEvent::Left:
        if (s_relay) {
            relayLost(SessionError::PeerLeft, "соперник вышел из комнаты");
            break;
        }
        if (s_state != SessionState::Connected) {
            s_state = SessionState::WaitingGuest;
            s_peerIdentity.clear();
            s_peerVersion = -1;
            s_p2pStarted = false;
        }
        break;
    case SignalEvent::Closed:
        if (s_relay) {
            if (s.text.empty())
                relayLost(SessionError::PeerLeft, "хост закрыл комнату");
            else
                relayLost(SessionError::ServerLost, "связь с сервером: " + s.text);
            break;
        }
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
    s_relayMine = relayOnly();
    std::string error;
    if (!s_relayMine && !listenP2P(&error)) {
        fail(SessionError::Setup, error);
        return false;
    }
    if (s_relayMine) {
        std::string why;
        if (!available(&why)) log("%s — соперник только через ретранслятор", why.c_str());
    }
    s_host = true;
    s_state = SessionState::Opening;
    openSignal();
    return true;
}

bool sessionJoin(const std::string& code) {
    sessionClose();
    s_relayMine = relayOnly();
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
    s_relayMine = s_relay = false;
    s_relayPartial.clear();
    s_relayPingAt = 0;
    s_relayRtt = -1;
}

SessionInfo sessionInfo() {
    SessionInfo info;
    info.state = s_state;
    info.host = s_host;
    info.code = s_code;
    info.peerName = s_peerName;
    info.error = s_error;
    info.ping = s_peer && s_state == SessionState::Connected ? ping(s_peer) : -1;
    info.serverPing = s_state == SessionState::Idle || s_state == SessionState::Failed ? -1 : signalPing();
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
        signalPingFrame();
    }
    std::string blob;
    while (takeSignal(&blob)) signalSend("ice:" + port::online::base64Encode(blob.data(), blob.size()));
    if (s_relay && s_state == SessionState::Connected) {
        long long now = nowMs();
        if (now >= s_relayPingNext && (!s_relayPingAt || now - s_relayPingAt > 5000)) {
            s_relayPingAt = now;
            s_relayPingNext = now + kRelayPingMs;
            signalSend("rp:" + std::to_string(now));
        }
    }

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

bool relaySend(const void* data, size_t size) {
    if (!s_relay || s_state != SessionState::Connected) return false;
    const char* bytes = static_cast<const char*>(data);
    size_t at = 0;
    do {
        size_t n = std::min(kRelayPart, size - at);
        bool last = at + n >= size;
        if (!signalSend((last ? "r:" : "R:") + port::online::base64Encode(bytes + at, n))) return false;
        at += n;
    } while (at < size);
    return true;
}

int relayPing() { return s_relayRtt; }

}
}
