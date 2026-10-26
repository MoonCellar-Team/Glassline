#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "port/net.h"
#include "ea_online/FBConnection.h"
#include "ea_online/FBFriend.h"
#include "ea_online/Mayhem/Challenge.h"
#include "ea_online/Mayhem/GetLeaderboardRequest.h"
#include "ea_online/Mayhem/GetStatRequest.h"
#include "ea_online/Mayhem/GetUserListRequest.h"
#include "ea_online/Mayhem/GetUserRequest.h"
#include "ea_online/Mayhem/PostChallenge.h"
#include "ea_online/Mayhem/PostStatRequest.h"
#include "ea_online/Mayhem/PostUserRequest.h"
#include "ea_online/Mayhem/Token.h"
#include "ea_online/Mayhem/UserList.h"
#include "ea_online/PendingChallenge.h"
#include "game/AppEngine.h"
#include "game/GameCenter.h"
#include "../platform/levels.h"
#include "json.h"
#include "port/online.h"

static const std::string& empty() {
    static const std::string s;
    return s;
}

bool FBConnection::isLoggedIn() { return false; }
void FBConnection::login() {}
int FBConnection::GetLoginResult() const { return LOGIN_CANCELED; }
void FBConnection::GetFriendIDs(std::vector<long long>& ids) const { ids.clear(); }
const FBFriend& FBConnection::GetFriend(int) const {
    static const FBFriend nobody;
    return nobody;
}

const std::string& FBFriend::GetName() const { return empty(); }
const std::string& FBFriend::GetPicURL() const { return empty(); }

namespace Mayhem {

static const int RESULT_FAILED = 2;

namespace {

namespace online = port::online;

struct UserInfo {
    std::string id, name;
    bool valid = false;
};

struct StatInfo {
    int level = -1;
    int timeMs = 0;
    int value = 0;
    std::string id;
    std::string data;
};

std::recursive_mutex s_mu;
std::map<const User*, std::shared_ptr<UserInfo>> s_users;
std::map<const Stat*, std::shared_ptr<StatInfo>> s_stats;

class BoardUser : public User {
public:
    explicit BoardUser(std::shared_ptr<UserInfo> info) {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_users[this] = std::move(info);
    }
    ~BoardUser() override {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_users.erase(this);
    }
};

class BoardStat : public Stat {
public:
    explicit BoardStat(std::shared_ptr<StatInfo> info) {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_stats[this] = std::move(info);
    }
    ~BoardStat() override {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_stats.erase(this);
    }
};

struct State {
    int result = Request::RESULT_PENDING;
    std::vector<std::unique_ptr<BoardUser>> users;
    std::vector<std::unique_ptr<BoardStat>> stats;
};

std::map<const Request*, std::shared_ptr<State>> s_requests;

std::shared_ptr<State> begin(const Request* r) {
    auto state = std::make_shared<State>();
    std::shared_ptr<State> old;
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    old = std::move(s_requests[r]);
    s_requests[r] = state;
    return state;
}

void finishState(const std::shared_ptr<State>& state, bool ok) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    state->result = ok ? Request::RESULT_SUCCESS : RESULT_FAILED;
}

void finish(const Request* r, bool ok) { finishState(begin(r), ok); }

const UserInfo* userInfo(const User* u) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_users.find(u);
    return it == s_users.end() ? nullptr : it->second.get();
}

const StatInfo* statInfo(const Stat* s) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_stats.find(s);
    return it == s_stats.end() ? nullptr : it->second.get();
}

void log(const char* what, const std::string& error) { fprintf(stderr, "medecomp: онлайн: %s: %s\n", what, error.c_str()); }

const char* const kGame = "rooftops";

std::string gameUrl() { return online::serverUrl() + "/games/" + kGame; }

std::string boardOf(int level) { return "level-" + std::to_string(level + 1); }

std::string boardFor(int level) {
    return port::platform::playerLevelPlaying() ? port::platform::playerLevelBoard() : boardOf(level);
}

int levelOfBoard(const std::string& board) {
    if (board.compare(0, 6, "level-") != 0) return -1;
    int n = atoi(board.c_str() + 6);
    return n >= 1 && n <= 100 ? n - 1 : -1;
}

int levelOf(const std::string& statId) {
    size_t at = statId.rfind("level");
    if (at == std::string::npos) return -1;
    int n = atoi(statId.c_str() + at + 5);
    return n >= 1 && n <= 100 ? n - 1 : -1;
}

std::shared_ptr<StatInfo> statFromJson(const online::Json& json, int level) {
    auto info = std::make_shared<StatInfo>();
    int board = levelOfBoard(json["board"].str());
    info->level = board >= 0 ? board : level;
    info->timeMs = int(json["score"].integer());
    info->id = json["entryId"].str();
    return info;
}

template <class Work>
void run(const std::shared_ptr<State>& state, Work work) {
    std::thread([state, work]() { finishState(state, work(*state)); }).detach();
}

}

int Request::GetResult() {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_requests.find(this);
    return it == s_requests.end() ? RESULT_FAILED : it->second->result;
}
const std::string& Request::GetErrorType() const { return empty(); }

Token::Token(const std::string&, const std::string&) { finish(this, !online::token().empty()); }
bool Token::IsValid() const { return const_cast<Token*>(this)->GetResult() == RESULT_SUCCESS; }
void Token::Refresh() {}

User::~User() {}
const std::string& User::GetMayhemID() const {
    const UserInfo* info = userInfo(this);
    return info ? info->id : empty();
}
bool User::IsValid() const {
    const UserInfo* info = userInfo(this);
    return info && info->valid;
}
const std::string& User::GetDisplayName() const {
    const UserInfo* info = userInfo(this);
    return info ? info->name : empty();
}

bool UserRequest::IsValid() { return User::IsValid(); }

GetUserRequest::GetUserRequest(bool) {
    auto info = std::make_shared<UserInfo>();
    {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_users[static_cast<const User*>(this)] = info;
    }
    auto state = begin(this);
    std::string token = online::token(), server = online::serverUrl();
    if (token.empty()) {
        finishState(state, false);
        return;
    }
    run(state, [info, token, server](State&) {
        online::Account account;
        bool revoked = false;
        std::string error;
        if (!online::fetchAccount(server, token, &account, &revoked, &error)) {
            log(revoked ? "вход отозван на сайте — войдите заново в лаунчере" : "аккаунт", error);
            return false;
        }
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        info->id = account.id;
        info->name = account.userName;
        info->valid = true;
        return true;
    });
}
GetUserRequest::GetUserRequest(long long, bool) { finish(this, false); }
PostUserRequest::PostUserRequest(const std::string&) { finish(this, false); }

Stat::~Stat() {}
int Stat::GetValue() const {
    const StatInfo* info = statInfo(this);
    if (!info) return 0;
    if (info->level < 0) return info->value;
    return AppEngine::getCanvas()->timeToMayhemScore(info->timeMs, info->level);
}
std::string Stat::GetStatID() const {
    const StatInfo* info = statInfo(this);
    return info ? info->id : std::string();
}
int Stat::GetDecodedData(void** out) const {
    const StatInfo* info = statInfo(this);
    if (!info || info->data.empty()) return 0;
    if (out) *out = const_cast<char*>(info->data.data());
    return int(info->data.size());
}

GetStatRequest::GetStatRequest(const std::string& statId, bool) {
    auto info = std::make_shared<StatInfo>();
    {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_stats[static_cast<const Stat*>(this)] = info;
    }
    auto state = begin(this);
    std::string url = gameUrl() + "/leaderboards/entries/" + statId + "/replay";
    run(state, [info, url](State&) {
        online::Response r = online::request("GET", url);
        online::Json json;
        std::string ghost;
        if (r.status != 200 || !online::parseJson(r.body, &json) || !online::base64Decode(json["replay"].str(), &ghost)) {
            log("призрак", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        auto parsed = statFromJson(json, -1);
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        *info = *parsed;
        info->data = std::move(ghost);
        return true;
    });
}

PostStatRequest::PostStatRequest(Token*, const std::string&, const std::string& statTypeCode, int value,
                                 int dataLength, const void* data) {
    auto state = begin(this);
    std::string token = online::token();
    int level = levelOf(statTypeCode);
    if (token.empty() || level < 0) {
        finishState(state, false);
        return;
    }
    if (boardFor(level).empty()) {
        finishState(state, false);
        return;
    }
    int timeMs = AppEngine::getCanvas()->mayhemScoreToTime(value, level);
    std::string body = "{\"score\":" + std::to_string(timeMs);
    if (data && dataLength > 0) body += ",\"replay\":\"" + online::base64Encode(data, size_t(dataLength)) + "\"";
    body += "}";
    std::string url = gameUrl() + "/leaderboards/" + boardFor(level);
    run(state, [url, body, token](State&) {
        online::Response r = online::request("PUT", url, body, {"Authorization: Bearer " + token});
        if (r.status != 200) {
            log("отправка времени", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        return true;
    });
}

GetLeaderboardRequest::GetLeaderboardRequest(Token*, const std::string& statId, int count) {
    auto state = begin(this);
    int level = levelOf(statId);
    if (level < 0) {
        finishState(state, false);
        return;
    }
    int limit = count < 1 ? 1 : count > 100 ? 100 : count;
    std::string url =
        gameUrl() + "/leaderboards/" + boardFor(level) + "?limit=" + std::to_string(limit);
    run(state, [url, level](State& out) {
        online::Response r = online::request("GET", url);
        online::Json json;
        if (r.status != 200 || !online::parseJson(r.body, &json)) {
            log("таблица лидеров", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        for (const online::Json& entry : json["entries"].items) {
            auto user = std::make_shared<UserInfo>();
            user->id = entry["userId"].str();
            user->name = entry["displayName"].str();
            user->valid = true;
            out.users.emplace_back(new BoardUser(user));
            out.stats.emplace_back(new BoardStat(statFromJson(entry, level)));
        }
        return true;
    });
}
GetLeaderboardRequest::GetLeaderboardRequest(Token*, const std::string&, const std::vector<long long>&) {
    finish(this, true);
}
int GetLeaderboardRequest::Size() const {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_requests.find(this);
    return it == s_requests.end() ? 0 : int(it->second->users.size());
}
User* GetLeaderboardRequest::GetUser(int index) const {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_requests.find(this);
    if (it == s_requests.end() || index < 0 || index >= int(it->second->users.size())) return NULL;
    return it->second->users[size_t(index)].get();
}
Stat* GetLeaderboardRequest::GetStat(int index) const {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_requests.find(this);
    if (it == s_requests.end() || index < 0 || index >= int(it->second->stats.size())) return NULL;
    return it->second->stats[size_t(index)].get();
}

namespace {

struct ChallengeInfo {
    std::string statTypeCode;
    int timeMs = 0;
    std::string data;
};

std::map<const Challenge*, std::shared_ptr<ChallengeInfo>> s_challenges;

const ChallengeInfo* challengeInfo(const Challenge* c) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_challenges.find(c);
    return it == s_challenges.end() ? nullptr : it->second.get();
}

std::shared_ptr<State> stateOf(const Request* r) {
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    auto it = s_requests.find(r);
    return it == s_requests.end() ? nullptr : it->second;
}

}

int Challenge::GetDecodedData(void** out) const {
    const ChallengeInfo* info = challengeInfo(this);
    if (!info || info->data.empty()) return 0;
    if (out) *out = const_cast<char*>(info->data.data());
    return int(info->data.size());
}
const std::string& Challenge::GetStatTypeCode() const {
    const ChallengeInfo* info = challengeInfo(this);
    return info ? info->statTypeCode : empty();
}
int Challenge::GetValue() const {
    const ChallengeInfo* info = challengeInfo(this);
    return info ? info->timeMs : 0;
}

GetChallenge::GetChallenge(Token*, const std::string&, const std::string& challengeId) {
    auto info = std::make_shared<ChallengeInfo>();
    {
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        s_challenges[static_cast<const Challenge*>(this)] = info;
    }
    auto state = begin(this);
    std::string token = online::token();
    if (token.empty()) {
        finishState(state, false);
        return;
    }
    std::string url = gameUrl() + "/challenges/" + challengeId;
    run(state, [info, url, token](State&) {
        online::Response r = online::request("GET", url, "", {"Authorization: Bearer " + token});
        online::Json json;
        std::string ghost;
        if (r.status != 200 || !online::parseJson(r.body, &json) ||
            !online::base64Decode(json["replay"].str(), &ghost)) {
            log("вызов", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        const std::string board = json["board"].str();
        info->statTypeCode = levelOfBoard(board) < 0 && !board.empty() && board[0] == 'u'
                                 ? "challenge_" + board
                                 : "challenge_level" + std::to_string(levelOfBoard(board) + 1);
        info->timeMs = int(json["score"].integer());
        info->data = std::move(ghost);
        return true;
    });
}

PostChallenge::PostChallenge(Token*, const std::string&, const std::string& statTypeCode, const std::string& userId,
                             int value, const std::string& comment, int dataLength, const void* data) {
    auto state = begin(this);
    std::string token = online::token();
    int level = levelOf(statTypeCode);
    if (token.empty() || level < 0 || !data || dataLength <= 0) {
        finishState(state, false);
        return;
    }
    std::string body = "{\"toUserId\":" + online::jsonString(userId) + ",\"board\":" + online::jsonString(boardFor(level)) +
                       ",\"score\":" + std::to_string(value) + ",\"comment\":" + online::jsonString(comment) +
                       ",\"replay\":\"" + online::base64Encode(data, size_t(dataLength)) + "\"}";
    std::string url = gameUrl() + "/challenges";
    run(state, [url, body, token](State&) {
        online::Response r = online::request("POST", url, body, {"Authorization: Bearer " + token});
        if (r.status != 200 && r.status != 201) {
            log("вызов другу", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        return true;
    });
}

GetUserListRequest::GetUserListRequest(const std::vector<long long>&) {
    auto state = begin(this);
    std::string token = online::token();
    if (token.empty()) {
        finishState(state, false);
        return;
    }
    std::string url = online::serverUrl() + "/account/friends";
    run(state, [url, token](State& out) {
        online::Response r = online::request("GET", url, "", {"Authorization: Bearer " + token});
        online::Json json;
        if (r.status != 200 || !online::parseJson(r.body, &json)) {
            log("друзья", online::errorOf(r.status, r.body, r.error));
            return false;
        }
        std::lock_guard<std::recursive_mutex> lock(s_mu);
        for (const online::Json& friendJson : json.items) {
            auto user = std::make_shared<UserInfo>();
            user->id = friendJson["id"].str();
            user->name = friendJson["userName"].str();
            user->valid = !user->id.empty();
            out.users.emplace_back(new BoardUser(user));
        }
        return true;
    });
}
UserList::~UserList() {}
int UserList::Size() const {
    auto state = stateOf(static_cast<const GetUserListRequest*>(this));
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    return state && state->result == Request::RESULT_SUCCESS ? int(state->users.size()) : 0;
}
User* UserList::GetMayhemUser(int index) {
    auto state = stateOf(static_cast<const GetUserListRequest*>(this));
    std::lock_guard<std::recursive_mutex> lock(s_mu);
    if (!state || index < 0 || index >= int(state->users.size())) return NULL;
    return state->users[size_t(index)].get();
}

}

namespace {

namespace online = port::online;

struct Incoming {
    std::string id, alert;
};

std::mutex s_incomingMu;
std::vector<Incoming> s_incoming;
std::atomic<bool> s_fetching(false);

void fetchIncoming(const std::string& url, const std::string& token) {
    online::Response r = online::request("GET", url, "", {"Authorization: Bearer " + token});
    online::Json json;
    if (r.status == 200 && online::parseJson(r.body, &json)) {
        std::lock_guard<std::mutex> lock(s_incomingMu);
        for (const online::Json& item : json.items) {
            Incoming in;
            in.id = item["id"].str();
            in.alert = item["fromName"].str();
            std::string comment = item["comment"].str();
            if (!comment.empty()) in.alert += ": " + comment;
            if (!in.id.empty()) s_incoming.push_back(in);
        }
    } else if (r.status != 401) {
        Mayhem::log("пришедшие вызовы", online::errorOf(r.status, r.body, r.error));
    }
    s_fetching = false;
}

}

namespace port {
namespace online {

void frame() {
    using Clock = std::chrono::steady_clock;
    static Clock::time_point next = Clock::now() + std::chrono::seconds(5);
    AppEngine* app = AppEngine::getCanvas();
    if (!app) return;
    {
        std::lock_guard<std::mutex> lock(s_incomingMu);
        for (const Incoming& in : s_incoming) {
            std::map<std::string, std::string> info;
            info["challengeId"] = in.id;
            info["alert"] = in.alert;
            app->receivedNotification(info);
        }
        if (!s_incoming.empty()) {
            app->savePendingChallenges();
            if (app->getPendingChallengeCount() == 0) app->loadPendingChallenges();
        }
        s_incoming.clear();
    }
    if (Clock::now() < next || s_fetching) return;
    next = Clock::now() + std::chrono::seconds(60);
    std::string current = token();
    if (current.empty()) return;
    s_fetching = true;
    std::string url = serverUrl() + "/games/" + Mayhem::kGame + "/challenges/incoming";
    std::thread(fetchIncoming, url, current).detach();
}

}
}

PendingChallenge::PendingChallenge(const std::string& id, const std::string& name) : m_id(id), m_name(name) {}
PendingChallenge::PendingChallenge(const PendingChallenge& other) : m_id(other.m_id), m_name(other.m_name) {}
PendingChallenge::~PendingChallenge() {}

GameCenter::GameCenter() {}
GameCenter::~GameCenter() {}
bool GameCenter::isGameCenterAPIAvailable() { return false; }
void GameCenter::authenticateLocalPlayer() {}

extern "C" void CleanIPSP(void*) {}

namespace port {
namespace online {

void startCheck() {
    if (!getenv("MEDECOMP_ONLINE_CHECK")) return;
    std::thread([] {
        std::string why;
        if (!available(&why)) {
            fprintf(stderr, "medecomp: онлайн: недоступен — %s\n", why.c_str());
            return;
        }
        const std::string server = serverUrl();
        Response health = request("GET", server + "/health");
        fprintf(stderr, "medecomp: онлайн: %s/health — %d %s\n", server.c_str(), health.status,
                health.status ? health.body.substr(0, 80).c_str() : health.error.c_str());
        Response board = request("GET", server + "/games/rooftops/leaderboards/level-1?limit=3");
        Json doc;
        int entries = parseJson(board.body, &doc) ? int(doc["entries"].items.size()) : -1;
        fprintf(stderr, "medecomp: онлайн: таблица level-1 — %d, записей %d\n", board.status, entries);
        std::string error;
        if (!port::net::signalOpen(&error)) {
            fprintf(stderr, "medecomp: онлайн: сигнальный сервер — нет: %s\n", error.c_str());
            return;
        }
        port::net::signalHost(27015);
        port::net::SignalReceived got;
        bool hosted = false;
        for (int i = 0; i < 500 && !hosted; i++) {
            while (!hosted && port::net::signalPoll(&got)) hosted = got.event == port::net::SignalEvent::Hosted;
            if (!hosted) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        fprintf(stderr, "medecomp: онлайн: сигнальный сервер — %s\n",
                hosted ? ("комната " + got.code).c_str() : "комнату не создал за 5 с");
        port::net::signalClose();
    }).detach();
}

}
}
