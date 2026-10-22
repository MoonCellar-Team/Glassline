#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace UI {
class Window;
}
namespace midp {
class Graphics;
}
class GameObject;
class Level;
class GameObjectPolice;
class MathVector;
class MEdgeMap;
namespace port {
namespace platform {
namespace skins {
struct Look;
}
}
}

namespace port {
namespace net {

using Peer = uint32_t;

enum class Event {
    Connected,
    Disconnected,
    Message,
};

struct Received {
    Event event;
    Peer peer;
    std::vector<uint8_t> data;
    std::string text;
};

bool available(std::string* why = nullptr);
bool host(uint16_t port, std::string* error = nullptr);
Peer connect(const std::string& address, std::string* error = nullptr);
bool libraryFound();
bool p2pAvailable();
std::string identity();
bool listenP2P(std::string* error = nullptr);
void setTurn(const std::vector<std::string>& servers, const std::string& user, const std::string& pass);
Peer connectP2P(const std::string& peerIdentity, std::string* error = nullptr);
void receivedSignal(const std::string& blob);
bool takeSignal(std::string* blob);
bool send(Peer peer, const void* data, size_t size, bool reliable);
bool poll(Received* out);
int ping(Peer peer);
void disconnect(Peer peer);
void shutdown();

void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logFile(const char* text);

struct SignalPeer {
    std::string name;
    std::string address;
    uint16_t port = 0;
    std::string endpoint() const;
};

enum class SignalEvent {
    Hosted,
    Joined,
    Peer,
    Message,
    Left,
    Closed,
    Error,
};

struct SignalTurn {
    std::vector<std::string> servers;
    std::string user, pass;
};

struct SignalReceived {
    SignalEvent event;
    std::string code;
    SignalPeer peer;
    std::string text;
    SignalTurn turn;
};

bool signalOpen(std::string* error = nullptr);
bool signalHost(uint16_t port);
bool signalJoin(const std::string& code, uint16_t port);
bool signalSend(const std::string& data);
bool signalPoll(SignalReceived* out);
void signalClose();

enum class SessionState {
    Idle,
    Opening,
    WaitingGuest,
    Connecting,
    Connected,
    Failed,
};

enum class SessionError {
    None,
    NoLibrary,
    NoP2P,
    Setup,
    ServerUnreachable,
    ServerLost,
    NoSuchGame,
    GameFull,
    HostLeft,
    NoRoute,
    ConnectFailed,
    PeerLeft,
    VersionMismatch,
};

const char* sessionErrorName(SessionError error);

struct SessionInfo {
    SessionState state;
    bool host;
    std::string code, peerName;
    SessionError error;
    int ping;
};

const uint8_t kLobbyMessage = 0x20;

const int kNetVersion = 4;

bool sessionHost();
bool sessionJoin(const std::string& code);
void sessionUseConnection(Peer peer, bool host);
void sessionClose();
SessionInfo sessionInfo();
Peer sessionPeer();
void sessionFrame();
bool sessionPoll(Received* out);
bool sessionTakeLobby(std::vector<uint8_t>* message);
bool sessionSend(const void* data, size_t size, bool reliable);

UI::Window* createLobbyWindow();
const int kLobbyChooseLevel = 3;
bool splitScreenChosen();

void raceFrame(int dt);
int remoteLevel();
bool remoteLocator(MathVector* at);
struct RaceChoice {
    int level, mode, timeLimit;
    bool combat;
    std::string playerLevel;
};
bool raceChoiceToJoin(RaceChoice* out);
void raceApplyChoice(const RaceChoice& choice);
void raceJoined();
bool raceHoldStart();
bool raceOpponent();
void raceUserRestart();
void raceUserQuit();
bool raceTogether();
bool raceIsHost();
bool raceHostChooses();
void raceChooseNext();
bool takeChooseNext();
void raceBrowsing(bool race, const Level* level, int timeLimit, bool combat, const char* playerLevel);
struct HostBrowsing {
    bool race = false;
    int level = -1;
    int timeLimit = 0;
    bool combat = false;
    std::string playerLevel;
};
bool raceHostBrowsing(HostBrowsing* out);
bool raceGettingLevel(int* percent, bool* failed);
bool shirtForGuest(const port::platform::skins::Look& look, bool mine);
bool rivalsNetwork();
int rivalsRemoteBags();
void rivalsBagTaken(GameObject* bag);
void rivalsGiveBag();
bool rivalsSyncTime(int* raceTime);
void enemyChanged(GameObjectPolice* cop);
void enemyAimBegin(GameObjectPolice* cop);
void enemyAimEnd();
GameObject* enemyBullet(MEdgeMap* map, float x, float y, float z, const MathVector& velocity);
void renderRace(midp::Graphics* g);
void renderRaceResult(midp::Graphics* g, int labelX, int timeX, int rowY, int centerX, int headlineY);

int selfTest(bool hostMode, const std::string& argument);
int selfTestRoom(bool hostMode, const std::string& argument);

}
}
