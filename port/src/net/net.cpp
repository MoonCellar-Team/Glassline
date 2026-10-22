#include "port/net.h"
#include "port/files.h"

#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <deque>
#include <mutex>
#include <random>
#include <string>

#include "steam/steamnetworkingcustomsignaling.h"
#include "steam/steamnetworkingsockets_flat.h"

namespace port {
namespace net {

namespace {

struct Api {
    bool (*init)(const SteamNetworkingIdentity*, SteamNetworkingErrMsg&);
    void (*kill)();
    decltype(&SteamAPI_ISteamNetworkingSockets_CreateListenSocketIP) listen;
    decltype(&SteamAPI_ISteamNetworkingSockets_ConnectByIPAddress) connectIP;
    decltype(&SteamAPI_ISteamNetworkingSockets_AcceptConnection) accept;
    decltype(&SteamAPI_ISteamNetworkingSockets_CloseConnection) close;
    decltype(&SteamAPI_ISteamNetworkingSockets_CloseListenSocket) closeListen;
    decltype(&SteamAPI_ISteamNetworkingSockets_SendMessageToConnection) sendTo;
    decltype(&SteamAPI_ISteamNetworkingSockets_CreatePollGroup) createGroup;
    decltype(&SteamAPI_ISteamNetworkingSockets_SetConnectionPollGroup) setGroup;
    decltype(&SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnPollGroup) receive;
    decltype(&SteamAPI_ISteamNetworkingSockets_RunCallbacks) runCallbacks;
    decltype(&SteamAPI_ISteamNetworkingSockets_GetConnectionRealTimeStatus) status;
    decltype(&SteamAPI_SteamNetworkingIPAddr_ParseString) parse;
    decltype(&SteamAPI_SteamNetworkingMessage_t_Release) release;
    decltype(&SteamAPI_ISteamNetworkingSockets_CreateListenSocketP2P) listenP2P;
    decltype(&SteamAPI_ISteamNetworkingSockets_ConnectP2PCustomSignaling) connectP2P;
    decltype(&SteamAPI_ISteamNetworkingSockets_ReceivedP2PCustomSignal) receivedSignal;
    decltype(&SteamAPI_SteamNetworkingIdentity_ToString) identityToString;
    decltype(&SteamAPI_ISteamNetworkingUtils_SetGlobalConfigValueString) setConfigString;
    decltype(&SteamAPI_ISteamNetworkingUtils_SetGlobalConfigValueInt32) setConfigInt;
};

std::recursive_mutex s_mu;
bool s_tried, s_ok;
bool s_found;
std::string s_why;
Api s_api;
ISteamNetworkingSockets* s_sockets;
HSteamNetPollGroup s_group = k_HSteamNetPollGroup_Invalid;
HSteamListenSocket s_listen = k_HSteamListenSocket_Invalid;
HSteamListenSocket s_listenP2P = k_HSteamListenSocket_Invalid;
std::deque<Received> s_events;
SteamNetworkingIdentity s_identity;
bool s_p2p;
bool s_gnsStderr;
ISteamNetworkingUtils* s_utils;
bool s_turnForced;

std::mutex s_signalMu;
std::deque<std::string> s_outSignals;

void queueSignal(const void* data, int size) {
    std::lock_guard<std::mutex> lock(s_signalMu);
    s_outSignals.emplace_back(static_cast<const char*>(data), size_t(size));
}

class OutSignaling : public ISteamNetworkingConnectionSignaling {
public:
    bool SendSignal(HSteamNetConnection, const SteamNetConnectionInfo_t&, const void* msg, int size) override {
        queueSignal(msg, size);
        return true;
    }
    void Release() override { delete this; }
};

class RecvContext : public ISteamNetworkingSignalingRecvContext {
public:
    ISteamNetworkingConnectionSignaling* OnConnectRequest(HSteamNetConnection, const SteamNetworkingIdentity&,
                                                          int) override {
        return s_listenP2P != k_HSteamListenSocket_Invalid ? new OutSignaling : nullptr;
    }
    void SendRejectionSignal(const SteamNetworkingIdentity&, const void* msg, int size) override {
        queueSignal(msg, size);
    }
};

void push(Event event, Peer peer, std::string text = std::string()) {
    Received r;
    r.event = event;
    r.peer = peer;
    r.text = std::move(text);
    s_events.push_back(std::move(r));
}

void onStatus(SteamNetConnectionStatusChangedCallback_t* info) {
    Peer peer = info->m_hConn;
    switch (info->m_info.m_eState) {
    case k_ESteamNetworkingConnectionState_Connecting:
        if (info->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid) {
            if (s_api.accept(s_sockets, peer) != k_EResultOK) {
                s_api.close(s_sockets, peer, 0, nullptr, false);
                break;
            }
            s_api.setGroup(s_sockets, peer, s_group);
        }
        break;
    case k_ESteamNetworkingConnectionState_Connected:
        push(Event::Connected, peer, info->m_info.m_szConnectionDescription);
        break;
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        push(Event::Disconnected, peer, info->m_info.m_szEndDebug);
        s_api.close(s_sockets, peer, 0, nullptr, false);
        break;
    default:
        break;
    }
}

SteamNetworkingConfigValue_t statusCallback() {
    SteamNetworkingConfigValue_t option;
    option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, reinterpret_cast<void*>(&onStatus));
    return option;
}

template <class F>
bool sym(void* lib, const char* name, F* out) {
    *out = reinterpret_cast<F>(dlsym(lib, name));
    if (!*out) s_why = std::string("нет ") + name;
    return *out != nullptr;
}

std::string exeDir() {
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    std::string dir = n > 0 ? std::string(exe, size_t(n)) : std::string(".");
    size_t slash = dir.rfind('/');
    return slash == std::string::npos ? std::string(".") : dir.substr(0, slash);
}

bool load() {
    if (s_tried) return s_ok;
    s_tried = true;
    void* lib = nullptr;
#ifdef _WIN32
    std::string path = exeDir() + "/libGameNetworkingSockets.dll";
#elif defined(__ANDROID__)
    std::string path = "libGameNetworkingSockets.so";
#else
    std::string path = exeDir() + "/lib/libGameNetworkingSockets.so";
#endif
    if (const char* forced = getenv("MEDECOMP_GNS"))
        if (*forced) path = forced;
    lib = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        const char* error = dlerror();
        s_why = "нет библиотеки GameNetworkingSockets: " + (error ? std::string(error) : path) +
                " (собирается вместе с игрой — port/src/net/gns.cmake)";
        return false;
    }
    log("GameNetworkingSockets — %s", path.c_str());
    s_found = true;
    using Accessor = ISteamNetworkingSockets* (*)();
    Accessor accessor = nullptr;
    for (const char* name : {"SteamNetworkingSockets_LibV12", "SteamAPI_SteamNetworkingSockets_v009"})
        if ((accessor = reinterpret_cast<Accessor>(dlsym(lib, name)))) break;
    bool ok = sym(lib, "GameNetworkingSockets_Init", &s_api.init) && sym(lib, "GameNetworkingSockets_Kill", &s_api.kill) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_CreateListenSocketIP", &s_api.listen) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_ConnectByIPAddress", &s_api.connectIP) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_AcceptConnection", &s_api.accept) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_CloseConnection", &s_api.close) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_CloseListenSocket", &s_api.closeListen) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_SendMessageToConnection", &s_api.sendTo) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_CreatePollGroup", &s_api.createGroup) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_SetConnectionPollGroup", &s_api.setGroup) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnPollGroup", &s_api.receive) &&
              sym(lib, "SteamAPI_ISteamNetworkingSockets_RunCallbacks", &s_api.runCallbacks) &&
              sym(lib, "SteamAPI_SteamNetworkingIPAddr_ParseString", &s_api.parse) &&
              sym(lib, "SteamAPI_SteamNetworkingMessage_t_Release", &s_api.release);
    s_api.status = reinterpret_cast<decltype(s_api.status)>(dlsym(lib, "SteamAPI_ISteamNetworkingSockets_GetConnectionRealTimeStatus"));
    using UtilsAccessor = ISteamNetworkingUtils* (*)();
    UtilsAccessor utilsAccessor = nullptr;
    for (const char* name : {"SteamNetworkingUtils_LibV4", "SteamAPI_SteamNetworkingUtils_v003"})
        if ((utilsAccessor = reinterpret_cast<UtilsAccessor>(dlsym(lib, name)))) break;
    s_p2p = utilsAccessor &&
            sym(lib, "SteamAPI_ISteamNetworkingSockets_CreateListenSocketP2P", &s_api.listenP2P) &&
            sym(lib, "SteamAPI_ISteamNetworkingSockets_ConnectP2PCustomSignaling", &s_api.connectP2P) &&
            sym(lib, "SteamAPI_ISteamNetworkingSockets_ReceivedP2PCustomSignal", &s_api.receivedSignal) &&
            sym(lib, "SteamAPI_SteamNetworkingIdentity_ToString", &s_api.identityToString) &&
            sym(lib, "SteamAPI_ISteamNetworkingUtils_SetGlobalConfigValueString", &s_api.setConfigString) &&
            sym(lib, "SteamAPI_ISteamNetworkingUtils_SetGlobalConfigValueInt32", &s_api.setConfigInt);
    if (!ok || !accessor) {
        if (!accessor) s_why = "нет SteamNetworkingSockets_LibV12";
        return false;
    }
    std::random_device random;
    char name[32];
    snprintf(name, sizeof name, "p-%08x%08x", random(), random());
    s_identity.Clear();
    s_identity.SetGenericString(name);
    SteamNetworkingErrMsg error;
    error[0] = 0;
    if (!s_api.init(&s_identity, error)) {
        s_why = std::string("GameNetworkingSockets_Init: ") + error;
        return false;
    }
    s_sockets = accessor();
    if (s_p2p) {
        ISteamNetworkingUtils* utils = utilsAccessor();
        s_utils = utils;
        const char* stun = getenv("MEDECOMP_STUN");
        s_api.setConfigString(utils, k_ESteamNetworkingConfig_P2P_STUN_ServerList,
                              stun && *stun ? stun : "stun.l.google.com:19302,stun.cloudflare.com:3478");
        const char* ice = getenv("MEDECOMP_ICE");
        bool relayOnly = ice && strcmp(ice, "relay") == 0;
        s_api.setConfigInt(utils, k_ESteamNetworkingConfig_P2P_Transport_ICE_Enable,
                           relayOnly ? k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Relay
                                     : k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_All);
        if (relayOnly) log("ICE — только через ретранслятор (MEDECOMP_ICE=relay)");
        s_api.setConfigInt(utils, k_ESteamNetworkingConfig_SendBufferSize, 20 << 20);
        const char* level = getenv("MEDECOMP_NET_LOG");
        s_gnsStderr = level && atoi(level) >= 3;
        auto debugOutput = reinterpret_cast<decltype(&SteamAPI_ISteamNetworkingUtils_SetDebugOutputFunction)>(
            dlsym(lib, "SteamAPI_ISteamNetworkingUtils_SetDebugOutputFunction"));
        ESteamNetworkingSocketsDebugOutputType detail =
            s_gnsStderr ? k_ESteamNetworkingSocketsDebugOutputType_Verbose : k_ESteamNetworkingSocketsDebugOutputType_Msg;
        if (debugOutput) {
            debugOutput(utils, detail, [](ESteamNetworkingSocketsDebugOutputType, const char* text) {
                std::string line = std::string("gns: ") + text;
                logFile(line.c_str());
                if (s_gnsStderr) fprintf(stderr, "%s\n", line.c_str());
            });
            s_api.setConfigInt(utils, k_ESteamNetworkingConfig_LogLevel_P2PRendezvous, detail);
        }
        if (const char* turn = getenv("MEDECOMP_TURN")) {
            std::string v = turn;
            size_t a = v.find(','), b = a == std::string::npos ? a : v.find(',', a + 1);
            if (b != std::string::npos) {
                setTurn({v.substr(0, a)}, v.substr(a + 1, b - a - 1), v.substr(b + 1));
                s_turnForced = true;
            }
        }
    }
    s_group = s_sockets ? s_api.createGroup(s_sockets) : k_HSteamNetPollGroup_Invalid;
    if (!s_sockets || s_group == k_HSteamNetPollGroup_Invalid) {
        s_why = "GameNetworkingSockets: нет сокетов";
        return false;
    }
    s_ok = true;
    return true;
}

}

namespace {
std::mutex s_logLock;
FILE* s_logFile;
bool s_logOpened;
const std::chrono::steady_clock::time_point s_logStart = std::chrono::steady_clock::now();

void writeLog(const char* line) {
    if (!s_logOpened) {
        s_logOpened = true;
        std::string path = port::files::dataDir() + "/net.log";
        rename(path.c_str(), (port::files::dataDir() + "/net.prev.log").c_str());
        s_logFile = fopen(path.c_str(), "w");
    }
    if (!s_logFile) return;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - s_logStart).count();
    fprintf(s_logFile, "%9.3f %s\n", t, line);
    fflush(s_logFile);
}
}

void setTurn(const std::vector<std::string>& servers, const std::string& user, const std::string& pass) {
    if (s_turnForced || !s_utils) return;
    std::string list, users, passes;
    for (std::string server : servers) {
        if (server.compare(0, 5, "turn:") == 0) server = server.substr(5);
        server = server.substr(0, server.find('?'));
        if (server.empty()) continue;
        list += (list.empty() ? "" : ",") + server;
        users += (users.empty() ? "" : ",") + user;
        passes += (passes.empty() ? "" : ",") + pass;
    }
    s_api.setConfigString(s_utils, k_ESteamNetworkingConfig_P2P_TURN_ServerList, list.c_str());
    s_api.setConfigString(s_utils, k_ESteamNetworkingConfig_P2P_TURN_UserList, users.c_str());
    s_api.setConfigString(s_utils, k_ESteamNetworkingConfig_P2P_TURN_PassList, passes.c_str());
    log("ретранслятор TURN: %s", list.empty() ? "нет" : list.c_str());
}

void log(const char* fmt, ...) {
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    std::lock_guard<std::mutex> lock(s_logLock);
    fprintf(stderr, "medecomp: сеть: %s\n", line);
    writeLog(line);
}

void logFile(const char* text) {
    std::lock_guard<std::mutex> lock(s_logLock);
    writeLog(text);
}

bool available(std::string* why) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    bool ok = load();
    if (!ok && why) *why = s_why;
    return ok;
}

bool host(uint16_t port, std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!load()) {
        if (error) *error = s_why;
        return false;
    }
    if (s_listen != k_HSteamListenSocket_Invalid) s_api.closeListen(s_sockets, s_listen);
    SteamNetworkingIPAddr address;
    address.Clear();
    address.m_port = port;
    SteamNetworkingConfigValue_t option = statusCallback();
    s_listen = s_api.listen(s_sockets, address, 1, &option);
    if (s_listen == k_HSteamListenSocket_Invalid) {
        if (error) *error = "порт " + std::to_string(port) + " не открыть";
        return false;
    }
    return true;
}

Peer connect(const std::string& where, std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!load()) {
        if (error) *error = s_why;
        return 0;
    }
    SteamNetworkingIPAddr address;
    address.Clear();
    if (!s_api.parse(&address, where.c_str()) || address.m_port == 0) {
        if (error) *error = "адрес — «IP:порт»: " + where;
        return 0;
    }
    SteamNetworkingConfigValue_t option = statusCallback();
    HSteamNetConnection peer = s_api.connectIP(s_sockets, address, 1, &option);
    if (peer == k_HSteamNetConnection_Invalid) {
        if (error) *error = "не подключиться к " + where;
        return 0;
    }
    s_api.setGroup(s_sockets, peer, s_group);
    return peer;
}

bool libraryFound() {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    load();
    return s_found;
}

bool p2pAvailable() {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    return load() && s_p2p;
}

std::string identity() {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!load() || !s_p2p) return std::string();
    char text[SteamNetworkingIdentity::k_cchMaxString];
    s_api.identityToString(&s_identity, text, sizeof text);
    return text;
}

bool listenP2P(std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!load() || !s_p2p) {
        if (error) *error = s_ok ? "в библиотеке нет P2P (ICE)" : s_why;
        return false;
    }
    if (s_listenP2P != k_HSteamListenSocket_Invalid) return true;
    SteamNetworkingConfigValue_t option = statusCallback();
    s_listenP2P = s_api.listenP2P(s_sockets, 0, 1, &option);
    if (s_listenP2P == k_HSteamListenSocket_Invalid) {
        if (error) *error = "P2P-приём не открыть";
        return false;
    }
    return true;
}

Peer connectP2P(const std::string& peerIdentity, std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!load() || !s_p2p) {
        if (error) *error = s_ok ? "в библиотеке нет P2P (ICE)" : s_why;
        return 0;
    }
    SteamNetworkingIdentity peerId;
    peerId.Clear();
    if (peerIdentity.compare(0, 4, "str:") != 0 || !peerId.SetGenericString(peerIdentity.c_str() + 4)) {
        if (error) *error = "идентификатор пира не разобрать: " + peerIdentity;
        return 0;
    }
    SteamNetworkingConfigValue_t option = statusCallback();
    HSteamNetConnection peer = s_api.connectP2P(s_sockets, new OutSignaling, &peerId, 0, 1, &option);
    if (peer == k_HSteamNetConnection_Invalid) {
        if (error) *error = "P2P-подключение не начать";
        return 0;
    }
    s_api.setGroup(s_sockets, peer, s_group);
    return peer;
}

void receivedSignal(const std::string& blob) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!s_ok || !s_p2p || blob.empty()) return;
    RecvContext context;
    s_api.receivedSignal(s_sockets, blob.data(), int(blob.size()), &context);
}

bool takeSignal(std::string* blob) {
    std::lock_guard<std::mutex> lock(s_signalMu);
    if (s_outSignals.empty()) return false;
    *blob = std::move(s_outSignals.front());
    s_outSignals.pop_front();
    return true;
}

bool send(Peer peer, const void* data, size_t size, bool reliable) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!s_ok || peer == 0) return false;
    int flags = reliable ? k_nSteamNetworkingSend_ReliableNoNagle : k_nSteamNetworkingSend_UnreliableNoNagle;
    return s_api.sendTo(s_sockets, peer, data, uint32(size), flags, nullptr) == k_EResultOK;
}

bool poll(Received* out) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!s_ok) return false;
    if (s_events.empty()) {
        s_api.runCallbacks(s_sockets);
        SteamNetworkingMessage_t* messages[32];
        int count = s_api.receive(s_sockets, s_group, messages, 32);
        for (int i = 0; i < count; i++) {
            Received r;
            r.event = Event::Message;
            r.peer = messages[i]->m_conn;
            const uint8_t* bytes = static_cast<const uint8_t*>(messages[i]->m_pData);
            r.data.assign(bytes, bytes + messages[i]->m_cbSize);
            s_events.push_back(std::move(r));
            s_api.release(messages[i]);
        }
    }
    if (s_events.empty()) return false;
    *out = std::move(s_events.front());
    s_events.pop_front();
    return true;
}

int ping(Peer peer) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!s_ok || peer == 0 || !s_api.status) return -1;
    SteamNetConnectionRealTimeStatus_t status;
    if (s_api.status(s_sockets, peer, &status, 0, nullptr) != k_EResultOK) return -1;
    return status.m_nPing;
}

void disconnect(Peer peer) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (s_ok && peer) s_api.close(s_sockets, peer, 0, "до встречи", true);
}

void shutdown() {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!s_ok) return;
    if (s_listen != k_HSteamListenSocket_Invalid) s_api.closeListen(s_sockets, s_listen);
    if (s_listenP2P != k_HSteamListenSocket_Invalid) s_api.closeListen(s_sockets, s_listenP2P);
    s_listen = k_HSteamListenSocket_Invalid;
    s_listenP2P = k_HSteamListenSocket_Invalid;
    s_api.kill();
    {
        std::lock_guard<std::mutex> signalLock(s_signalMu);
        s_outSignals.clear();
    }
    s_ok = false;
    s_tried = false;
    s_events.clear();
}

}
}
