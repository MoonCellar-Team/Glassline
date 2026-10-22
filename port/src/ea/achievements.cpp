#include "port/achievements.h"

#include <SDL.h>
#include <stdio.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>

#include "json.h"
#include "port/files.h"
#include "port/online.h"

namespace port {
namespace achievements {

namespace {

using online::Json;

std::mutex s_mutex;
std::atomic<bool> s_flushing{false};
std::atomic<bool> s_again{false};
std::string s_lastError;

const char* const kMetrics[] = {"deaths",  "kills",       "rivalKills",  "disarms",     "enemyFalls",  "bags",
                                "badLandings", "unk1c",   "runDist",     "wallRunDist", "climbDist",   "ziplineDist",
                                "balanceDist", "slideDist", "fallDist",  "timeMillis"};

bool readText(const std::string& path, std::string* out) {
    std::vector<unsigned char> data;
    if (!files::read(path, &data)) return false;
    out->assign(data.begin(), data.end());
    return true;
}

bool writeText(const std::string& path, const std::string& text) {
    std::string tmp = path + ".part";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(text.data(), 1, text.size(), f) == text.size();
    ok = fclose(f) == 0 && ok;
    if (ok) {
        remove(path.c_str());
        ok = rename(tmp.c_str(), path.c_str()) == 0;
    }
    return ok;
}

std::string catalogDir() {
    char* base = SDL_GetBasePath();
    std::string dir = base ? base : "./";
    SDL_free(base);
    while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) dir.pop_back();
    for (int i = 0; i < 6 && !dir.empty(); i++) {
        std::string candidate = dir + "/config/achievements";
        if (files::stat(candidate + "/" + kGame + ".json").exists) return candidate;
        size_t slash = dir.find_last_of("/\\");
        if (slash == std::string::npos || slash == 0) break;
        dir.erase(slash);
    }
    return std::string();
}

std::map<std::string, std::string> texts(const Json& j) {
    std::map<std::string, std::string> out;
    for (const auto& field : j.fields) out[field.first] = field.second.str();
    return out;
}

std::vector<Entry> loadCatalog() {
    std::vector<Entry> out;
    std::string dir = catalogDir(), text;
    Json doc;
    if (dir.empty() || !readText(dir + "/" + kGame + ".json", &text) || !online::parseJson(text, &doc)) {
        fprintf(stderr, "medecomp: ачивки: каталога config/achievements/%s.json нет — без своих уведомлений\n", kGame);
        return out;
    }
    for (const Json& a : doc["achievements"].items) {
        Entry e;
        e.key = a["key"].str();
        e.title = texts(a["title"]);
        e.description = texts(a["description"]);
        e.index = int(a["trigger"]["index"].integer(-1));
        e.metric = a["trigger"]["metric"].str();
        e.target = int(a["progress"]["target"].integer(0));
        e.unit = a["progress"]["unit"].str();
        readText(dir + "/" + kGame + "/" + a["icon"].str(), &e.icon);
        if (!e.key.empty() && e.title.count("en")) out.push_back(e);
    }
    return out;
}

std::string iso(long long unixTime) {
    time_t t = time_t(unixTime);
    struct tm utc;
#ifdef _WIN32
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    char buf[32];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buf;
}

struct Item {
    std::string key;
    long long unlockedAt = -1;
    int current = -1;
};

std::string queuePath() { return files::dataDir() + "/achievements_queue.json"; }
std::string sentPath() { return files::dataDir() + "/achievements_sent.json"; }

std::vector<Item> loadQueue() {
    std::vector<Item> out;
    std::string text;
    Json doc;
    if (!readText(queuePath(), &text) || !online::parseJson(text, &doc)) return out;
    for (const Json& j : doc["items"].items) {
        Item it;
        it.key = j["key"].str();
        it.unlockedAt = j["unlockedAt"].integer(-1);
        it.current = int(j["current"].integer(-1));
        if (!it.key.empty()) out.push_back(it);
    }
    return out;
}

void saveQueue(const std::vector<Item>& items) {
    std::string text = "{\"items\":[";
    for (size_t i = 0; i < items.size(); i++) {
        if (i) text += ",";
        text += "{\"key\":" + online::jsonString(items[i].key) + ",\"unlockedAt\":" +
                std::to_string(items[i].unlockedAt) + ",\"current\":" + std::to_string(items[i].current) + "}";
    }
    text += "]}\n";
    if (!writeText(queuePath(), text)) fprintf(stderr, "medecomp: ачивки: очередь не записана (%s)\n", queuePath().c_str());
}

struct Sent {
    std::string account;
    std::map<std::string, std::pair<bool, int>> items;
};

Sent loadSent(const std::string& account) {
    Sent out;
    out.account = account;
    std::string text;
    Json doc;
    if (!readText(sentPath(), &text) || !online::parseJson(text, &doc) || doc["account"].str() != account) return out;
    for (const auto& f : doc["sent"].fields)
        out.items[f.first] = {f.second["unlocked"].type == Json::Bool && f.second["unlocked"].boolean,
                              int(f.second["current"].integer(-1))};
    return out;
}

void saveSent(const Sent& sent) {
    std::string text = "{\"account\":" + online::jsonString(sent.account) + ",\"sent\":{";
    bool first = true;
    for (const auto& kv : sent.items) {
        if (!first) text += ",";
        first = false;
        text += online::jsonString(kv.first) + ":{\"unlocked\":" + (kv.second.first ? "true" : "false") +
                ",\"current\":" + std::to_string(kv.second.second) + "}";
    }
    text += "}}\n";
    writeText(sentPath(), text);
}

void merge(std::vector<Item>* items, const Item& add) {
    for (Item& it : *items)
        if (it.key == add.key) {
            if (add.unlockedAt >= 0 && (it.unlockedAt < 0 || (it.unlockedAt == 0 && add.unlockedAt > 0)))
                it.unlockedAt = add.unlockedAt;
            it.current = std::max(it.current, add.current);
            return;
        }
    items->push_back(add);
}

void flushOnce(const std::string& server, const std::string& token, const std::string& account) {
    std::vector<Item> items;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        items = loadQueue();
    }
    std::string error;
    for (const Item& it : items) {
        std::string body = "{";
        if (it.unlockedAt >= 0) {
            body += "\"unlocked\":true";
            if (it.unlockedAt > 0) body += ",\"unlockedAt\":" + online::jsonString(iso(it.unlockedAt));
        }
        if (it.current >= 0) body += std::string(body.size() > 1 ? "," : "") + "\"current\":" + std::to_string(it.current);
        body += "}";
        online::Response r = online::request("PUT", server + "/games/" + kGame + "/achievements/" + it.key, body,
                                             {"Authorization: Bearer " + token});
        bool drop = r.status == 200 || r.status == 404;
        if (!drop) {
            error = online::errorOf(r.status, r.body, r.error);
            break;
        }
        std::lock_guard<std::mutex> lock(s_mutex);
        if (r.status == 200) {
            Sent sent = loadSent(account);
            auto& s = sent.items[it.key];
            if (it.unlockedAt >= 0) s.first = true;
            s.second = std::max(s.second, it.current);
            saveSent(sent);
        } else {
            fprintf(stderr, "medecomp: ачивки: на сервере нет «%s» — убрана из очереди\n", it.key.c_str());
        }
        std::vector<Item> now = loadQueue();
        now.erase(std::remove_if(now.begin(), now.end(),
                                 [&](const Item& q) {
                                     return q.key == it.key && q.unlockedAt == it.unlockedAt && q.current <= it.current;
                                 }),
                  now.end());
        saveQueue(now);
    }
    std::lock_guard<std::mutex> lock(s_mutex);
    s_lastError = error;
    if (!error.empty()) fprintf(stderr, "medecomp: ачивки: не отправлены: %s\n", error.c_str());
}

}

const std::string& Entry::titleIn(bool russian) const {
    auto it = title.find(russian ? "ru" : "en");
    return it != title.end() ? it->second : title.at("en");
}

const std::string& Entry::descriptionIn(bool russian) const {
    static const std::string empty;
    auto it = description.find(russian ? "ru" : "en");
    if (it != description.end()) return it->second;
    it = description.find("en");
    return it != description.end() ? it->second : empty;
}

const std::vector<Entry>& catalog() {
    static const std::vector<Entry> entries = loadCatalog();
    return entries;
}

const Entry* byIndex(int index) {
    for (const Entry& e : catalog())
        if (e.index == index) return &e;
    return nullptr;
}

const Entry* byKey(const std::string& key) {
    for (const Entry& e : catalog())
        if (e.key == key) return &e;
    return nullptr;
}

Local readLocal() {
    Local out;
    std::vector<unsigned char> d;
    if (!files::read(files::dataDir() + "/Documents/achievements_data", &d) || d.size() < 68) return out;
    auto u32 = [&](size_t at) {
        return uint32_t(d[at]) << 24 | uint32_t(d[at + 1]) << 16 | uint32_t(d[at + 2]) << 8 | uint32_t(d[at + 3]);
    };
    int count = int(u32(0));
    if (count < 0 || count > 1000 || d.size() < size_t(68 + count)) return out;
    for (int i = 0; i < 16; i++) {
        uint32_t raw = u32(4 + size_t(i) * 4);
        int value;
        if (i >= 8 && i < 15) {
            float f;
            memcpy(&f, &raw, sizeof f);
            value = std::isfinite(f) && f > 0 ? int(std::floor(f)) : 0;
        } else {
            value = int(raw);
        }
        out.metrics[kMetrics[i]] = value;
    }
    for (int i = 0; i < count; i++) out.unlocked.push_back(d[68 + size_t(i)] != 0);
    out.ok = true;
    return out;
}

int currentOf(const Entry& entry, const Local& local) {
    if (entry.metric.empty() || !local.ok) return -1;
    auto it = local.metrics.find(entry.metric);
    return it == local.metrics.end() ? -1 : it->second;
}

void queueUnlock(const std::string& key, long long unixTime) {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::vector<Item> items = loadQueue();
    Item it;
    it.key = key;
    it.unlockedAt = std::max(0LL, unixTime);
    merge(&items, it);
    saveQueue(items);
}

void queueProgress(const std::string& key, int current) {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::vector<Item> items = loadQueue();
    Item it;
    it.key = key;
    it.current = current;
    merge(&items, it);
    saveQueue(items);
}

void queueFromSave(const std::string& account) {
    if (account.empty()) return;
    Local local = readLocal();
    if (!local.ok) return;
    std::lock_guard<std::mutex> lock(s_mutex);
    Sent sent = loadSent(account);
    std::vector<Item> items = loadQueue();
    bool changed = false;
    for (const Entry& e : catalog()) {
        auto s = sent.items.find(e.key);
        bool sentUnlock = s != sent.items.end() && s->second.first;
        bool unlocked = e.index >= 0 && size_t(e.index) < local.unlocked.size() && local.unlocked[size_t(e.index)];
        if (unlocked && !sentUnlock) {
            Item it;
            it.key = e.key;
            it.unlockedAt = 0;
            merge(&items, it);
            changed = true;
        } else if (!unlocked && e.target > 0) {
            int current = currentOf(e, local);
            int sentCurrent = s != sent.items.end() ? s->second.second : -1;
            if (current > sentCurrent && current > 0) {
                Item it;
                it.key = e.key;
                it.current = current;
                merge(&items, it);
                changed = true;
            }
        }
    }
    if (changed) saveQueue(items);
}

int pending() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return int(loadQueue().size());
}

void flushAsync(const std::string& server, const std::string& token, const std::string& account) {
    if (token.empty() || server.empty()) return;
    s_again = true;
    if (s_flushing.exchange(true)) return;
    std::thread([server, token, account] {
        while (s_again.exchange(false)) flushOnce(server, token, account);
        s_flushing = false;
    }).detach();
}

bool flushing() { return s_flushing; }

std::string lastError() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_lastError;
}

bool fetchMine(const std::string& server, const std::string& token, std::map<std::string, Remote>* out,
               std::string* error) {
    online::Response r = online::request("GET", server + "/games/" + kGame + "/achievements/me", std::string(),
                                         {"Authorization: Bearer " + token});
    Json doc;
    if (r.status != 200 || !online::parseJson(r.body, &doc)) {
        if (error) *error = online::errorOf(r.status, r.body, r.error);
        return false;
    }
    out->clear();
    for (const Json& a : doc["achievements"].items) {
        Remote m;
        m.unlocked = a["unlocked"].type == Json::Bool && a["unlocked"].boolean;
        m.unlockedAt = a["unlockedAt"].str();
        m.current = int(a["current"].integer(-1));
        (*out)[a["key"].str()] = m;
    }
    return true;
}

}
}
