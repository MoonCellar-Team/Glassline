#include "levels.h"

#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <set>

#include "cloud.h"
#include "json.h"
#include "port/files.h"
#include "port/online.h"

namespace port {
namespace online {

namespace {

const char* const kLevels = "/games/rooftops/levels";

std::string levelsDir() { return port::files::dataDir() + "/levels"; }

std::string escapeUrl(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += char(c);
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

bool readInfo(const std::string& id, Json* info) {
    std::vector<unsigned char> json;
    return port::files::read(levelsDir() + "/" + id + "/level.json", &json) &&
           parseJson(std::string(json.begin(), json.end()), info) && info->type == Json::Object;
}

bool writeFile(const std::string& path, const std::string& data) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

std::string modeJson(const std::string& mode, long long timeLimit) {
    return mode == "bags" ? ",\n  \"mode\": \"bags\",\n  \"timeLimit\": " + std::to_string(timeLimit)
                          : std::string(",\n  \"mode\": \"race\"");
}

std::string infoJson(const std::string& name, const std::string& author, const std::string& palette, long long version,
                     const std::string& origin, const std::string& description, const NetLevel* server,
                     const std::string& mode, long long timeLimit, const std::string& supports = std::string()) {
    std::string s = "{\n  \"name\": " + jsonString(name) + ",\n  \"author\": " + jsonString(author) +
                    ",\n  \"palette\": " + jsonString(palette) + ",\n  \"version\": " + std::to_string(version) +
                    ",\n  \"origin\": " + jsonString(origin) + ",\n  \"description\": " + jsonString(description) +
                    modeJson(mode, timeLimit);
    if (!supports.empty()) s += ",\n  \"supports\": " + jsonString(supports);
    if (server)
        s += ",\n  \"server\": {\"id\": " + jsonString(server->id) + ", \"version\": " + std::to_string(server->version) +
             ", \"board\": " + jsonString(server->board) +
             (server->authorId.empty() ? std::string() : ", \"authorId\": " + jsonString(server->authorId)) + "}";
    return s + "\n}\n";
}

long long unixTime(const std::string& iso) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, sec = 0;
    if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &sec) != 6) return 0;
    y -= mo <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400, yoe = y - era * 400;
    const long long doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + doe - 719468) * 86400 + h * 3600 + mi * 60 + sec;
}

NetLevel levelOf(const Json& j) {
    NetLevel l;
    l.id = j["id"].str();
    l.name = j["name"].str();
    l.description = j["description"].str();
    l.base = j["base"].str();
    l.mode = j["mode"].str() == "bags" ? "bags" : "race";
    l.timeLimit = int(j["timeLimit"].integer());
    l.author = j["authorName"].str();
    l.board = j["board"].str();
    l.version = int(j["version"].integer());
    l.ratings = int(j["ratings"].integer());
    l.downloads = int(j["downloads"].integer());
    l.rating = j["rating"].number;
    l.authorId = j["authorId"].str();
    l.hash = j["hash"].str();
    l.updatedAt = unixTime(j["updatedAt"].str());
    l.hidden = j["hidden"].boolean;
    return l;
}

NetLevel serverOf(const Json& info) {
    NetLevel l;
    l.id = info["server"]["id"].str();
    l.version = int(info["server"]["version"].integer());
    l.board = info["server"]["board"].str();
    l.authorId = info["server"]["authorId"].str();
    l.author = info["author"].str();
    return l;
}

bool rewriteInfo(const std::string& id, const Json& info, const std::string& origin, const NetLevel* server) {
    return writeFile(levelsDir() + "/" + id + "/level.json",
                     infoJson(info["name"].str(id), server ? server->author : info["author"].str(), info["palette"].str(),
                              info["version"].integer(1), origin, info["description"].str(), server,
                              info["mode"].str("race"), info["timeLimit"].integer(), info["supports"].str()));
}

struct Known {
    bool loaded = false;
    std::string server, account;
    std::map<std::string, NetLevel> levels;
    std::set<std::string> missing;
};
std::mutex s_knownMu;
Known s_known;
std::map<std::string, std::pair<std::string, std::string>> s_sha;

std::string knownPath() { return port::files::dataDir() + "/cache/levels-online.json"; }

Known& known() {
    if (s_known.loaded) return s_known;
    s_known.loaded = true;
    std::vector<unsigned char> text;
    Json j;
    if (!port::files::read(knownPath(), &text) || !parseJson(std::string(text.begin(), text.end()), &j) ||
        j["server"].str() != serverUrl())
        return s_known;
    s_known.server = j["server"].str();
    s_known.account = j["account"].str();
    for (const Json& l : j["levels"].items) {
        NetLevel n = levelOf(l);
        n.updatedAt = l["updatedAt"].integer();
        s_known.levels[n.id] = n;
    }
    for (const Json& m : j["missing"].items) s_known.missing.insert(m.str());
    return s_known;
}

void saveKnown() {
    std::string s = "{\n  \"server\": " + jsonString(s_known.server) + ",\n  \"account\": " + jsonString(s_known.account) +
                    ",\n  \"levels\": [";
    bool first = true;
    for (const auto& kv : s_known.levels) {
        const NetLevel& l = kv.second;
        s += std::string(first ? "" : ",") + "\n    {\"id\": " + jsonString(l.id) + ", \"version\": " + std::to_string(l.version) +
             ", \"board\": " + jsonString(l.board) + ", \"authorId\": " + jsonString(l.authorId) +
             ", \"hash\": " + jsonString(l.hash) + ", \"updatedAt\": " + std::to_string(l.updatedAt) +
             ", \"hidden\": " + (l.hidden ? "true" : "false") + "}";
        first = false;
    }
    s += "\n  ],\n  \"missing\": [";
    first = true;
    for (const std::string& m : s_known.missing) {
        s += std::string(first ? "" : ", ") + jsonString(m);
        first = false;
    }
    mkdir((port::files::dataDir() + "/cache").c_str(), 0755);
    writeFile(knownPath(), s + "]\n}\n");
}

void remember(const NetLevel& level) {
    std::lock_guard<std::mutex> lock(s_knownMu);
    Known& k = known();
    k.server = serverUrl();
    if (k.account.empty()) k.account = level.authorId;
    k.levels[level.id] = level;
    k.missing.erase(level.id);
    saveKnown();
}

bool otherAccount(const Json& info) {
    std::string author = info["server"]["authorId"].str();
    if (author.empty()) return false;
    std::lock_guard<std::mutex> lock(s_knownMu);
    const Known& k = known();
    return !k.account.empty() && k.account != author;
}

std::string localSha(const std::string& id, long long* mtime) {
    struct stat st;
    std::string path = levelsDir() + "/" + id + "/level.bin";
    if (stat(path.c_str(), &st) != 0) return std::string();
    *mtime = st.st_mtime;
    std::string stamp = std::to_string(st.st_mtime) + ":" + std::to_string(st.st_size);
    {
        std::lock_guard<std::mutex> lock(s_knownMu);
        auto it = s_sha.find(id);
        if (it != s_sha.end() && it->second.first == stamp) return it->second.second;
    }
    std::vector<unsigned char> data;
    if (!port::files::read(path, &data)) return std::string();
    std::string sha = cloud::sha256(std::string(data.begin(), data.end()));
    std::lock_guard<std::mutex> lock(s_knownMu);
    s_sha[id] = {stamp, sha};
    return sha;
}

std::string twinKey(const std::string& name, const std::string& base, const std::string& mode) {
    size_t a = name.find_first_not_of(" \t"), b = name.find_last_not_of(" \t");
    std::string key = a == std::string::npos ? std::string() : name.substr(a, b - a + 1);
    for (char& c : key) c = char(tolower((unsigned char)c));
    return key + "\n" + base + "\n" + (mode == "bags" ? "bags" : "race");
}

bool ok(const Response& r, int status, const char* what, Json* body, std::string* error) {
    if (r.status == status && (!body || parseJson(r.body, body))) return true;
    *error = errorOf(r.status, r.body, r.error);
    fprintf(stderr, "medecomp: уровни на сервере: %s: %s\n", what, error->c_str());
    return false;
}

std::vector<std::string> auth() { return {"Authorization: Bearer " + token(), "Content-Type: application/json"}; }

}

bool listLevels(const std::string& sort, const std::string& q, int offset, int limit, std::vector<NetLevel>* out,
                int* total, std::string* error, const std::string& mode) {
    std::string url = serverUrl() + kLevels + "?sort=" + escapeUrl(sort) + "&offset=" + std::to_string(offset) +
                      "&limit=" + std::to_string(limit);
    if (!q.empty()) url += "&q=" + escapeUrl(q);
    if (!mode.empty()) url += "&mode=" + escapeUrl(mode);
    Json body;
    if (!ok(request("GET", url), 200, "список", &body, error)) return false;
    out->clear();
    for (const Json& j : body["levels"].items) out->push_back(levelOf(j));
    *total = int(body["total"].integer());
    return true;
}

bool getLevel(const std::string& id, NetLevel* out, bool* missing, std::string* error) {
    Response r = request("GET", serverUrl() + kLevels + "/" + escapeUrl(id));
    *missing = r.status == 404;
    Json body;
    if (!ok(r, 200, "уровень", &body, error)) return false;
    *out = levelOf(body);
    return true;
}

bool downloadLevel(const NetLevel& level, std::string* localId, std::string* error) {
    Json body;
    if (!ok(request("GET", serverUrl() + kLevels + "/" + level.id + "/file"), 200, "скачивание", &body, error))
        return false;
    std::string data;
    if (!base64Decode(body["data"].str(), &data)) return *error = "плохой ответ сервера", false;
    *localId = "net-" + level.id;
    std::string dir = levelsDir() + "/" + *localId;
    mkdir(levelsDir().c_str(), 0755);
    mkdir(dir.c_str(), 0755);
    NetLevel server = level;
    server.version = int(body["version"].integer(level.version));
    if (!writeFile(dir + "/level.bin", data) ||
        !writeFile(dir + "/level.json",
                   infoJson(level.name, level.author, level.base, server.version, "online", level.description, &server,
                            level.mode, level.timeLimit)))
        return *error = "не записать " + dir, false;
    return true;
}

bool uploadable(const std::string& localId) {
    Json info;
    if (!readInfo(localId, &info)) return false;
    std::string origin = info["origin"].str();
    return origin == "empty" || (origin == "uploaded" && !info["server"]["id"].str().empty() && !otherAccount(info));
}

bool myLevels(std::vector<NetLevel>* out, std::string* authorId, std::string* error) {
    Json body;
    if (!ok(request("GET", serverUrl() + kLevels + "/mine", std::string(), auth()), 200, "свои уровни", &body, error))
        return false;
    out->clear();
    for (const Json& j : body["levels"].items) out->push_back(levelOf(j));
    *authorId = body["authorId"].str();
    return true;
}

bool syncUploaded(int* changed, std::string* error) {
    *changed = 0;
    std::vector<std::string> ids;
    for (const std::string& id : port::files::list(levelsDir()))
        if (port::files::stat(levelsDir() + "/" + id + "/level.json").exists) ids.push_back(id);
    if (token().empty()) {
        bool any = false, failed = false;
        for (const std::string& id : ids) {
            std::string serverId = uploadedId(id);
            if (serverId.empty()) continue;
            NetLevel level;
            bool missing = false;
            std::string why;
            if (!getLevel(serverId, &level, &missing, &why) && !missing) {
                failed = true, *error = why;
                continue;
            }
            any = true;
            std::lock_guard<std::mutex> lock(s_knownMu);
            Known& k = known();
            k.server = serverUrl();
            if (missing) k.levels.erase(serverId), k.missing.insert(serverId);
            else k.levels[serverId] = level, k.missing.erase(serverId);
        }
        if (any) {
            std::lock_guard<std::mutex> lock(s_knownMu);
            saveKnown();
        }
        return !failed;
    }
    std::vector<NetLevel> mine;
    std::string account;
    if (!myLevels(&mine, &account, error)) return false;
    std::map<std::string, NetLevel> byId;
    for (const NetLevel& l : mine) byId[l.id] = l;
    std::set<std::string> taken;
    std::map<std::string, std::vector<std::string>> unlinked;
    for (const std::string& id : ids) {
        Json info;
        if (!readInfo(id, &info)) continue;
        std::string origin = info["origin"].str();
        if (origin == "empty") {
            unlinked[twinKey(info["name"].str(id), info["palette"].str(), info["mode"].str())].push_back(id);
            continue;
        }
        if (origin != "uploaded") continue;
        NetLevel was = serverOf(info);
        auto it = byId.find(was.id);
        if (it != byId.end()) {
            taken.insert(was.id);
            long long mtime = 0;
            NetLevel now = was;
            now.authorId = it->second.authorId;
            if (!it->second.hash.empty() && localSha(id, &mtime) == it->second.hash)
                now.version = it->second.version, now.board = it->second.board;
            if ((now.authorId != was.authorId || now.version != was.version) && rewriteInfo(id, info, "uploaded", &now)) ++*changed;
            continue;
        }
        if (!was.authorId.empty() && was.authorId != account) continue;
        if (was.authorId.empty()) {
            NetLevel level;
            bool missing = false;
            std::string why;
            if (!getLevel(was.id, &level, &missing, &why)) {
                if (!missing) continue;
            } else {
                was.authorId = level.authorId;
                if (rewriteInfo(id, info, "uploaded", &was)) ++*changed;
                continue;
            }
        }
        if (rewriteInfo(id, info, "empty", nullptr)) ++*changed;
        fprintf(stderr, "medecomp: уровни на сервере: %s там больше нет — снова не выложен\n", id.c_str());
    }
    std::map<std::string, std::vector<const NetLevel*>> free;
    for (const NetLevel& l : mine)
        if (!taken.count(l.id)) free[twinKey(l.name, l.base, l.mode)].push_back(&l);
    for (auto& kv : unlinked) {
        auto there = free.find(kv.first);
        if (there == free.end()) continue;
        std::vector<std::string>& locals = kv.second;
        std::vector<const NetLevel*>& remote = there->second;
        auto link = [&](const std::string& id, const NetLevel* level) {
            Json info;
            if (!readInfo(id, &info) || !rewriteInfo(id, info, "uploaded", level)) return;
            ++*changed;
            fprintf(stderr, "medecomp: уровни на сервере: %s — это %s («%s»), выложенный раньше\n", id.c_str(),
                    level->id.c_str(), level->name.c_str());
        };
        for (auto l = locals.begin(); l != locals.end();) {
            long long mtime = 0;
            std::string sha = localSha(*l, &mtime);
            auto r = std::find_if(remote.begin(), remote.end(), [&](const NetLevel* n) { return !sha.empty() && n->hash == sha; });
            if (r == remote.end()) {
                ++l;
                continue;
            }
            link(*l, *r);
            remote.erase(r);
            l = locals.erase(l);
        }
        if (locals.size() == 1 && remote.size() == 1) link(locals[0], remote[0]);
    }
    std::lock_guard<std::mutex> lock(s_knownMu);
    Known& k = known();
    k.server = serverUrl();
    k.account = account;
    k.levels = byId;
    k.missing.clear();
    saveKnown();
    return true;
}

OnlineState onlineState(const std::string& localId) {
    OnlineState out;
    Json info;
    if (!readInfo(localId, &info) || info["origin"].str() != "uploaded") return out;
    if (otherAccount(info)) {
        out.state = Online::OtherAccount;
        return out;
    }
    std::string serverId = info["server"]["id"].str();
    NetLevel there;
    {
        std::lock_guard<std::mutex> lock(s_knownMu);
        const Known& k = known();
        if (k.missing.count(serverId)) {
            out.state = Online::Missing;
            return out;
        }
        auto it = k.levels.find(serverId);
        if (it == k.levels.end()) return out;
        there = it->second;
    }
    out.hidden = there.hidden;
    long long mtime = 0;
    std::string sha = localSha(localId, &mtime);
    if (there.hash.empty() || sha.empty()) return out;
    out.state = sha == there.hash ? Online::Same : Online::Differs;
    out.newerThere = there.updatedAt > mtime;
    return out;
}

bool takeLevel(const std::string& localId, std::string* error) {
    Json info;
    if (!readInfo(localId, &info)) return *error = "нет файлов уровня", false;
    std::string serverId = uploadedId(localId);
    if (serverId.empty()) return *error = "уровень не выложен", false;
    NetLevel level;
    bool missing = false;
    if (!getLevel(serverId, &level, &missing, error)) return false;
    Json body;
    if (!ok(request("GET", serverUrl() + kLevels + "/" + escapeUrl(serverId) + "/file"), 200, "скачивание", &body, error))
        return false;
    std::string data;
    if (!base64Decode(body["data"].str(), &data)) return *error = "плохой ответ сервера", false;
    level.version = int(body["version"].integer(level.version));
    if (level.hash.empty()) level.hash = cloud::sha256(data);
    std::string dir = levelsDir() + "/" + localId;
    if (!writeFile(dir + "/level.bin", data) ||
        !writeFile(dir + "/level.json", infoJson(level.name, level.author, level.base, info["version"].integer(1), "uploaded",
                                                 level.description, &level, level.mode, level.timeLimit, info["supports"].str())))
        return *error = "не записать " + dir, false;
    remember(level);
    return true;
}

bool uploadLevel(const std::string& localId, std::string* error) {
    Json info;
    std::vector<unsigned char> data;
    if (!readInfo(localId, &info) || !port::files::read(levelsDir() + "/" + localId + "/level.bin", &data))
        return *error = "нет файлов уровня", false;
    if (!uploadable(localId)) return *error = "выкладывать можно только уровни, собранные с нуля", false;
    if (token().empty()) return *error = "нужен вход в MoonCellar", false;
    std::string serverId = info["server"]["id"].str();
    if (serverId.empty()) {
        int changed = 0;
        std::string ignored;
        Json linked;
        if (syncUploaded(&changed, &ignored) && readInfo(localId, &linked) && linked["origin"].str() == "uploaded") {
            info = linked;
            serverId = info["server"]["id"].str();
            if (onlineState(localId).state == Online::Same) return true;
        }
    }
    bool bags = info["mode"].str() == "bags";
    std::string body = "{\"name\":" + jsonString(info["name"].str(localId)) +
                       ",\"description\":" + jsonString(info["description"].str()) +
                       ",\"base\":" + jsonString(info["palette"].str()) +
                       ",\"mode\":\"" + (bags ? "bags" : "race") + "\",\"timeLimit\":" +
                       std::to_string(bags ? info["timeLimit"].integer(180000) : 0) +
                       ",\"data\":\"" + base64Encode(data.data(), data.size()) + "\"}";
    bool update = !serverId.empty();
    Json reply;
    if (!ok(request(update ? "PUT" : "POST", serverUrl() + kLevels + (update ? "/" + serverId : ""), body, auth()),
            update ? 200 : 201, "выкладка", &reply, error))
        return false;
    NetLevel server = levelOf(reply);
    if (server.hash.empty()) server.hash = cloud::sha256(std::string(data.begin(), data.end()));
    remember(server);
    return writeFile(levelsDir() + "/" + localId + "/level.json",
                     infoJson(info["name"].str(localId), server.author, info["palette"].str(),
                              info["version"].integer(1), "uploaded", info["description"].str(), &server,
                              info["mode"].str("race"), info["timeLimit"].integer(), info["supports"].str()))
               ? true
               : (*error = "не записать level.json", false);
}

std::string uploadedId(const std::string& localId) {
    Json info;
    if (!readInfo(localId, &info) || info["origin"].str() != "uploaded") return std::string();
    return info["server"]["id"].str();
}

bool deleteLevel(const std::string& serverId, std::string* error) {
    if (token().empty()) return *error = "нужен вход в MoonCellar", false;
    Response r = request("DELETE", serverUrl() + kLevels + "/" + escapeUrl(serverId), std::string(), auth());
    if (r.status != 404 && !ok(r, 200, "удаление", nullptr, error)) return false;
    {
        std::lock_guard<std::mutex> lock(s_knownMu);
        known().levels.erase(serverId);
        saveKnown();
    }
    for (const std::string& id : port::files::list(levelsDir())) {
        Json info;
        if (uploadedId(id) != serverId || !readInfo(id, &info)) continue;
        if (!writeFile(levelsDir() + "/" + id + "/level.json",
                       infoJson(info["name"].str(id), info["author"].str(), info["palette"].str(),
                                info["version"].integer(1), "empty", info["description"].str(), nullptr,
                                info["mode"].str("race"), info["timeLimit"].integer(), info["supports"].str())))
            return *error = "не записать level.json", false;
    }
    return true;
}

bool rateLevel(const std::string& id, int stars, std::string* error) {
    return ok(request("PUT", serverUrl() + kLevels + "/" + id + "/rating", "{\"stars\":" + std::to_string(stars) + "}",
                      auth()),
              200, "оценка", nullptr, error);
}

bool reportLevel(const std::string& id, const std::string& reason, std::string* error) {
    return ok(request("POST", serverUrl() + kLevels + "/" + id + "/report", "{\"reason\":" + jsonString(reason) + "}",
                      auth()),
              200, "жалоба", nullptr, error);
}

bool levelLeaderboard(const std::string& board, int limit, std::vector<std::pair<std::string, int>>* out,
                      std::string* error) {
    Json body;
    if (!ok(request("GET", serverUrl() + "/games/rooftops/leaderboards/" + escapeUrl(board) +
                               "?limit=" + std::to_string(limit)),
            200, "таблица уровня", &body, error))
        return false;
    out->clear();
    for (const Json& e : body["entries"].items) out->push_back({e["displayName"].str(), int(e["score"].integer())});
    return true;
}

}
}
