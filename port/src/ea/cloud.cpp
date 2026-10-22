#include "cloud.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "../platform/settings.h"
#include "json.h"
#include "port/files.h"
#include "port/online.h"

namespace settings = port::platform::settings;

namespace port {
namespace online {
namespace cloud {

namespace {

const char* const kGame = "rooftops";

std::string s_server, s_token, s_user;
bool s_syncSkins = true;

void capture() {
    s_server = serverUrl();
    s_token = token();
    s_user = userName();
    s_syncSkins = skinsEnabled();
}

}

std::string sha256(const std::string& data) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98,
        0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8,
        0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
        0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2,
    };
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::string m = data;
    uint64_t bits = uint64_t(data.size()) * 8;
    m += char(0x80);
    while (m.size() % 64 != 56) m += char(0);
    for (int i = 7; i >= 0; i--) m += char((bits >> (i * 8)) & 0xff);
    auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    for (size_t chunk = 0; chunk < m.size(); chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(m.data() + chunk + size_t(i) * 4);
            w[i] = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g, g = f, f = e, e = d + t1, d = c, c = b, b = a, a = t1 + t2;
        }
        h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e, h[5] += f, h[6] += g, h[7] += hh;
    }
    char out[65];
    for (int i = 0; i < 8; i++) snprintf(out + i * 8, 9, "%08x", h[i]);
    return std::string(out, 64);
}

namespace {

struct Entry {
    std::string sha;
    long long size = 0;
    long long time = 0;
};
using Files = std::map<std::string, Entry>;

struct Manifest {
    bool valid = false;
    long long revision = -1;
    Files files;
};

struct Outcome {
    int down = 0, up = 0, removed = 0;
    int skipped = 0;
    bool conflict = false, localWon = false, first = false;
    bool pending = false;
    long long localTime = 0, remoteTime = 0;
    std::string backup, error;
};

enum Choice { Ask, Newer, TakeRemote, TakeLocal };

bool readFile(const std::string& path, std::string* out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    std::ostringstream s;
    s << in.rdbuf();
    *out = s.str();
    return true;
}

void makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++)
        if (i == path.size() || path[i] == '/') mkdir(path.substr(0, i).c_str(), 0755);
}

bool writeFile(const std::string& path, const std::string& data) {
    size_t slash = path.find_last_of('/');
    if (slash != std::string::npos) makeDirs(path.substr(0, slash));
    std::string part = path + ".part";
    {
        std::ofstream out(part.c_str(), std::ios::binary);
        out.write(data.data(), std::streamsize(data.size()));
        if (!out) return false;
    }
    remove(path.c_str());
    return rename(part.c_str(), path.c_str()) == 0;
}

long long parseIso(const std::string& s) {
    struct tm t;
    memset(&t, 0, sizeof t);
    if (sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
        return 0;
    t.tm_year -= 1900;
    t.tm_mon -= 1;
#ifdef _WIN32
    return (long long)_mkgmtime(&t);
#else
    return (long long)timegm(&t);
#endif
}

bool validPath(const std::string& path) {
    int parts = 0;
    size_t start = 0;
    while (true) {
        size_t end = path.find('/', start);
        std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part.empty() || part.size() > 100 || part == "." || part == "..") return false;
        for (char c : part)
            if (!(isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-')) return false;
        if (++parts > 4) return false;
        if (end == std::string::npos) return true;
        start = end + 1;
    }
}

bool partial(const std::string& path) { return path.size() >= 5 && path.compare(path.size() - 5, 5, ".part") == 0; }

bool keepSave(const std::string& path) {
    return path.find('/') == std::string::npos && path != "pendingChallenges" && !partial(path);
}

bool hasProgress(const std::map<std::string, Entry>& files) {
    for (const auto& f : files)
        if (f.first == "LevelProgress" || f.first == "achievements_data" || f.first.compare(0, 5, "Ghost") == 0) return true;
    return false;
}

bool keepLevel(const std::string& path) {
    size_t slash = path.find('/');
    return slash != std::string::npos && slash > 0 && path.find('/', slash + 1) == std::string::npos && !partial(path);
}

bool keepPrefs(const std::string& path) { return path == "prefs.json" || path == "lut.png"; }

bool keepSkin(const std::string& path) {
    size_t slash = path.find('/');
    return path.compare(0, 5, "skin-") == 0 && slash != std::string::npos &&
           path.find('/', slash + 1) == std::string::npos && !partial(path);
}

void scan(const std::string& root, const std::string& rel, int depth, bool (*keep)(const std::string&),
          const Files& known, Files* out) {
    for (const std::string& name : port::files::list(rel.empty() ? root : root + "/" + rel)) {
        std::string path = rel.empty() ? name : rel + "/" + name;
        std::string full = root + "/" + path;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (depth < 1) scan(root, path, depth + 1, keep, known, out);
            continue;
        }
        if (!keep(path)) continue;
        if (!validPath(path)) {
            fprintf(stderr, "medecomp: облако — %s не в аккаунт (имя не из латиницы, цифр и ._-)\n", full.c_str());
            continue;
        }
        Entry e;
        e.size = (long long)st.st_size;
        e.time = (long long)st.st_mtime;
        auto k = known.find(path);
        if (k != known.end() && k->second.size == e.size && k->second.time == e.time) {
            e.sha = k->second.sha;
        } else {
            std::string data;
            if (!readFile(full, &data)) continue;
            e.sha = sha256(data);
        }
        (*out)[path] = e;
    }
}

std::string manifestPath(const std::string& bucket) { return port::files::dataDir() + "/cache/cloud-" + bucket + ".json"; }

Manifest loadManifest(const std::string& bucket) {
    Manifest m;
    std::string text;
    Json j;
    if (!readFile(manifestPath(bucket), &text) || !parseJson(text, &j) || j.type != Json::Object) return m;
    if (j["server"].str() != s_server || j["user"].str() != s_user) return m;
    m.valid = true;
    m.revision = j["revision"].integer(-1);
    for (const auto& f : j["files"].fields) {
        Entry e;
        e.sha = f.second["sha"].str();
        e.size = f.second["size"].integer();
        e.time = f.second["time"].integer();
        m.files[f.first] = e;
    }
    return m;
}

void saveManifest(const std::string& bucket, long long revision, const Files& local) {
    std::ostringstream s;
    s << "{\"server\":" << jsonString(s_server) << ",\"user\":" << jsonString(s_user) << ",\"revision\":" << revision
      << ",\"files\":{";
    bool first = true;
    for (const auto& f : local) {
        s << (first ? "" : ",") << jsonString(f.first) << ":{\"sha\":\"" << f.second.sha << "\",\"size\":" << f.second.size
          << ",\"time\":" << f.second.time << "}";
        first = false;
    }
    s << "}}";
    makeDirs(port::files::dataDir() + "/cache");
    writeFile(manifestPath(bucket), s.str());
}

std::string bucketUrl(const std::string& bucket) { return s_server + "/games/" + kGame + "/storage/" + bucket; }
std::vector<std::string> authHeaders() { return {"Authorization: Bearer " + s_token}; }

bool listRemote(const std::string& bucket, Files* out, long long* revision, long long* maxFile, std::string* error) {
    Response r = request("GET", bucketUrl(bucket), std::string(), authHeaders());
    Json j;
    if (r.status != 200 || !parseJson(r.body, &j)) return *error = errorOf(r.status, r.body, r.error), false;
    *revision = j["revision"].integer();
    *maxFile = j["maxFileBytes"].integer();
    for (const Json& f : j["files"].items) {
        Entry e;
        e.sha = f["sha256"].str();
        e.size = f["size"].integer();
        e.time = parseIso(f["updatedAt"].str());
        (*out)[f["path"].str()] = e;
    }
    return true;
}

bool fetchFile(const std::string& bucket, const std::string& path, std::string* data, std::string* error) {
    Response r = request("GET", bucketUrl(bucket) + "/file?path=" + path, std::string(), authHeaders(), 120000);
    if (r.status != 200) return *error = errorOf(r.status, r.body, r.error), false;
    *data = std::move(r.body);
    return true;
}

void slow() {
    if (const char* ms = getenv("MEDECOMP_CLOUD_SLOW")) std::this_thread::sleep_for(std::chrono::milliseconds(atoi(ms)));
}

bool putFile(const std::string& bucket, const std::string& path, const std::string& data, long long* revision,
             std::string* error) {
    slow();
    std::vector<std::string> headers = authHeaders();
    headers.push_back("Content-Type: application/octet-stream");
    Response r = request("PUT", bucketUrl(bucket) + "/file?path=" + path, data, headers, 120000);
    Json j;
    if (r.status != 200 || !parseJson(r.body, &j)) return *error = errorOf(r.status, r.body, r.error), false;
    *revision = j["revision"].integer();
    return true;
}

bool deleteFile(const std::string& bucket, const std::string& path, long long* revision, std::string* error) {
    Response r = request("DELETE", bucketUrl(bucket) + "/file?path=" + path, std::string(), authHeaders());
    Json j;
    if (r.status == 404) return true;
    if (r.status != 200 || !parseJson(r.body, &j)) return *error = errorOf(r.status, r.body, r.error), false;
    *revision = j["revision"].integer();
    return true;
}

std::atomic<int> s_done(0), s_total(0);

int transfers(const Files& from, const Files& to) {
    int n = 0;
    for (const auto& f : from) {
        auto t = to.find(f.first);
        if (t == to.end() || t->second.sha != f.second.sha) n++;
    }
    return n;
}

bool sameFiles(const Files& a, const Files& b) {
    if (a.size() != b.size()) return false;
    for (const auto& f : a) {
        auto it = b.find(f.first);
        if (it == b.end() || it->second.sha != f.second.sha) return false;
    }
    return true;
}

long long newest(const Files& files) {
    long long t = 0;
    for (const auto& f : files) t = std::max(t, f.second.time);
    return t;
}

std::string stamp() {
    time_t now = time(nullptr);
    struct tm t;
#ifdef _WIN32
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    char buf[32];
    strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &t);
    return buf;
}

bool download(const std::string& bucket, const std::string& dir, const Files& local, const Files& remote, bool prune,
              Outcome* o) {
    for (const auto& f : remote) {
        auto l = local.find(f.first);
        if (l != local.end() && l->second.sha == f.second.sha) continue;
        std::string data;
        if (!fetchFile(bucket, f.first, &data, &o->error)) return false;
        if (sha256(data) != f.second.sha) return o->error = "файл " + f.first + " пришёл повреждённым", false;
        if (!writeFile(dir + "/" + f.first, data)) return o->error = "не записать " + dir + "/" + f.first, false;
        o->down++;
        s_done++;
    }
    if (prune)
        for (const auto& f : local)
            if (!remote.count(f.first) && remove((dir + "/" + f.first).c_str()) == 0) o->removed++;
    return true;
}

bool upload(const std::string& bucket, const std::string& dir, const Files& local, const Files& remote, bool prune,
            long long maxFile, long long* revision, Outcome* o) {
    for (const auto& f : local) {
        auto r = remote.find(f.first);
        if (r != remote.end() && r->second.sha == f.second.sha) continue;
        if (f.second.size > maxFile) {
            fprintf(stderr, "medecomp: облако — %s больше %lld Б, в аккаунт не уходит\n", f.first.c_str(), maxFile);
            continue;
        }
        std::string data;
        if (!readFile(dir + "/" + f.first, &data)) continue;
        if (!putFile(bucket, f.first, data, revision, &o->error)) return false;
        o->up++;
        s_done++;
    }
    if (prune)
        for (const auto& f : remote)
            if (!local.count(f.first)) {
                if (!deleteFile(bucket, f.first, revision, &o->error)) return false;
                o->removed++;
            }
    return true;
}

void backupLocal(const std::string& dir, const Files& files, const std::string& to) {
    for (const auto& f : files) {
        std::string data;
        if (readFile(dir + "/" + f.first, &data)) writeFile(to + "/" + f.first, data);
    }
}

bool syncWhole(const std::string& bucket, const std::string& dir, bool (*keep)(const std::string&), Choice choice, Outcome* o) {
    Manifest m = loadManifest(bucket);
    Files local, remote;
    scan(dir, std::string(), 0, keep, m.files, &local);
    long long revision = 0, maxFile = 0;
    if (!listRemote(bucket, &remote, &revision, &maxFile, &o->error)) return false;
    if (!sameFiles(local, remote)) {
        bool localChanged = !m.valid || !sameFiles(local, m.files);
        bool remoteChanged = !m.valid || !sameFiles(remote, m.files);
        o->first = !m.valid;
        o->conflict = m.valid ? localChanged && remoteChanged : !remote.empty() && hasProgress(local);
        if (o->conflict && choice == Ask) {
            o->pending = true;
            o->localTime = newest(local);
            o->remoteTime = newest(remote);
            return true;
        }
        bool takeRemote;
        if (o->conflict)
            takeRemote = choice == TakeRemote || (choice == Newer && newest(remote) > newest(local));
        else
            takeRemote = m.valid ? remoteChanged : !remote.empty();
        std::string backups = port::files::dataDir() + "/backups/" + bucket + "-" + stamp();
        s_total += takeRemote ? transfers(remote, local) : transfers(local, remote);
        if (takeRemote) {
            if (o->conflict || !local.empty()) backupLocal(dir, local, o->backup = backups);
            if (!download(bucket, dir, local, remote, true, o)) return false;
        } else {
            o->localWon = true;
            if (o->conflict && !remote.empty()) download(bucket, o->backup = backups + "-account", Files(), remote, false, o);
            o->down = 0;
            if (!upload(bucket, dir, local, remote, true, maxFile, &revision, o)) return false;
        }
        local.clear();
        scan(dir, std::string(), 0, keep, Files(), &local);
    }
    saveManifest(bucket, revision, local);
    return true;
}

std::string folderOf(const std::string& path) {
    size_t slash = path.find('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

bool syncFiles(const std::string& bucket, const std::string& dir, bool (*keep)(const std::string&), bool firstFromAccount,
               Outcome* o, int maxFolders = 0) {
    Manifest m = loadManifest(bucket);
    Files local, remote;
    scan(dir, std::string(), 0, keep, m.files, &local);
    long long revision = 0, maxFile = 0;
    if (!listRemote(bucket, &remote, &revision, &maxFile, &o->error)) return false;
    Files toDown, toUp, dropLocal, dropRemote;
    std::map<std::string, bool> paths;
    for (const auto& f : local) paths[f.first] = true;
    for (const auto& f : remote) paths[f.first] = true;
    for (const auto& f : m.files) paths[f.first] = true;
    for (const auto& p : paths) {
        auto l = local.find(p.first), r = remote.find(p.first), b = m.files.find(p.first);
        std::string ls = l == local.end() ? "" : l->second.sha, rs = r == remote.end() ? "" : r->second.sha,
                    bs = b == m.files.end() ? "" : b->second.sha;
        if (ls == rs) continue;
        bool localSame = ls == bs, remoteSame = rs == bs;
        bool takeRemote = localSame ? true : remoteSame ? false
                                                         : (r != remote.end() ? r->second.time : 0) > (l != local.end() ? l->second.time : 0);
        if (!m.valid && firstFromAccount && r != remote.end()) takeRemote = true;
        if (takeRemote) {
            if (r != remote.end()) toDown[p.first] = r->second;
            else dropLocal[p.first] = l->second;
        } else {
            if (l != local.end()) toUp[p.first] = l->second;
            else dropRemote[p.first] = r->second;
        }
    }
    if (maxFolders > 0) {
        std::map<std::string, bool> held, skipped;
        for (const auto& f : remote)
            if (!dropRemote.count(f.first) && !folderOf(f.first).empty()) held[folderOf(f.first)] = true;
        std::vector<std::string> fresh;
        for (const auto& f : toUp) {
            std::string folder = folderOf(f.first);
            if (!folder.empty() && !held.count(folder) && (fresh.empty() || fresh.back() != folder)) fresh.push_back(folder);
        }
        std::sort(fresh.begin(), fresh.end(), [](const std::string& a, const std::string& b) {
            return a.size() != b.size() ? a.size() < b.size() : a < b;
        });
        fresh.erase(std::unique(fresh.begin(), fresh.end()), fresh.end());
        for (const std::string& folder : fresh) {
            if (int(held.size()) < maxFolders) held[folder] = true;
            else skipped[folder] = true;
        }
        for (auto it = toUp.begin(); it != toUp.end();) {
            if (skipped.count(folderOf(it->first))) it = toUp.erase(it);
            else ++it;
        }
        o->skipped = int(skipped.size());
    }
    s_total += int(toDown.size() + toUp.size());
    if (!download(bucket, dir, Files(), toDown, false, o)) return false;
    for (const auto& f : dropLocal)
        if (remove((dir + "/" + f.first).c_str()) == 0) o->removed++;
    for (const auto& f : dropRemote) {
        if (!deleteFile(bucket, f.first, &revision, &o->error)) return false;
        o->removed++;
    }
    if (!upload(bucket, dir, toUp, Files(), false, maxFile, &revision, o)) return false;
    for (const auto& f : dropLocal) {
        std::string folder = dir + "/" + f.first.substr(0, f.first.find('/'));
        if (port::files::list(folder).empty()) remove(folder.c_str());
    }
    local.clear();
    scan(dir, std::string(), 0, keep, Files(), &local);
    saveManifest(bucket, revision, local);
    return true;
}

const char* const kPortable[] = {
    "language", "volume", "music", "music_volume", "music_shuffle", "hrtf", "background_audio", "skin", "deadzone",
    "key.up", "key.down", "key.left", "key.right", "key.twoLeft", "key.twoRight", "key.tap", "key.tiltLeft",
    "key.tiltRight", "key.pause", "pad.up", "pad.down", "pad.left", "pad.right", "pad.twoLeft", "pad.twoRight",
    "pad.tap", "pad.tiltLeft", "pad.tiltRight", "pad.pause",
};

std::string prefsDir() { return port::files::dataDir() + "/cache/cloud-prefs"; }

void sameTime(const std::string& path, const std::string& like) {
    struct stat st;
    if (stat(like.c_str(), &st) != 0) return;
    struct utimbuf t;
    t.actime = st.st_atime;
    t.modtime = st.st_mtime;
    utime(path.c_str(), &t);
}

void writePrefs() {
    std::ostringstream s;
    s << "{";
    bool first = true;
    for (const char* key : kPortable) {
        std::string value = settings::get(key);
        if (value.empty()) continue;
        s << (first ? "" : ",") << "\n  " << jsonString(key) << ": " << jsonString(value);
        first = false;
    }
    s << "\n}\n";
    std::string dir = prefsDir(), old, data = port::files::dataDir();
    makeDirs(dir);
    if (!readFile(dir + "/prefs.json", &old) || old != s.str())
        if (writeFile(dir + "/prefs.json", s.str())) sameTime(dir + "/prefs.json", data + "/settings.json");
    std::string lut, staged;
    bool have = readFile(data + "/lut.png", &lut);
    if (have && (!readFile(dir + "/lut.png", &staged) || staged != lut))
        if (writeFile(dir + "/lut.png", lut)) sameTime(dir + "/lut.png", data + "/lut.png");
    if (!have) remove((dir + "/lut.png").c_str());
}

std::mutex s_mu;
std::atomic<bool> s_prefsChanged(false);
std::atomic<bool> s_levelsAgain(false);
Outcome s_conflict;
std::atomic<bool> s_busy(false), s_skinsChanged(false);
std::string s_status;

std::string describe(const Outcome& save, const Outcome& skins, bool saveOk, bool skinsOk, bool russian, bool withSave) {
    auto L = [russian](const char* ru, const char* en) { return std::string(russian ? ru : en); };
    std::string out;
    if (!withSave) {
        out = L("Сохранение ждёт кнопки «Синхронизировать»: прошлая игра закрылась, не отправив его.",
                "The save waits for Sync: the last game closed without sending it.");
    } else if (!saveOk) {
        out = L("Сохранение не сверено с аккаунтом: ", "The save is not synced with the account: ") + save.error;
    } else if (save.pending) {
        out = L("Сохранения здесь и в аккаунте разные — выберите, какое оставить.",
                "The save here and in the account differ — choose which one to keep.");
    } else if (save.conflict) {
        out = save.localWon ? L("Сохранения здесь и в аккаунте разошлись — оставлено здешнее, оно теперь в аккаунте; прежнее из аккаунта — копией: ",
                                "The save here and in the account differed — this computer's is kept and is in the account now; the account's copy: ")
                            : L("Сохранения здесь и в аккаунте разошлись — взято из аккаунта; здешнее — копией: ",
                                "The save here and in the account differed — the account's is used; this computer's copy: ");
        out += save.backup;
    } else if (save.down) {
        out = L("Сохранение из аккаунта загружено — игра продолжится с того же места.",
                "The save from the account is loaded — the game goes on where you left off.");
    } else if (save.up) {
        out = L("Сохранение отправлено в аккаунт.", "The save is in the account.");
    } else {
        out = L("Сохранение совпадает с аккаунтом.", "The save matches the account.");
    }
    if (!skinsOk) {
        out += " " + L("Скины не сверены: ", "Skins are not synced: ") + skins.error;
    } else if (skins.down || skins.up || skins.removed) {
        out += " " + L("Скины: из аккаунта ", "Skins: from the account ") + std::to_string(skins.down) + L(", в аккаунт ", ", to the account ") +
               std::to_string(skins.up) + (skins.removed ? L(", удалено ", ", removed ") + std::to_string(skins.removed) : std::string()) + ".";
    }
    if (skinsOk && skins.skipped)
        out += " " + L("Своих скинов больше ", "There are more than ") + std::to_string(kSkinLimit) +
               L(" — в аккаунт не ушли ещё ", " custom skins — not sent to the account: ") + std::to_string(skins.skipped) +
               L(". Удалите лишние (вкладка «Героиня») или выключите «Синхронизировать свои скины».",
                 ". Delete the extra ones (Heroine tab) or turn off Sync custom skins.");
    return out;
}

std::string unsyncedPath() { return port::files::dataDir() + "/cache/cloud-unsynced"; }

void runLevels() {
    Outcome levels;
    std::string dir = port::files::dataDir() + "/levels";
    makeDirs(dir);
    if (syncFiles("levels", dir, keepLevel, false, &levels))
        fprintf(stderr, "medecomp: облако — уровни: ↓%d ↑%d ×%d\n", levels.down, levels.up, levels.removed);
    else
        fprintf(stderr, "medecomp: облако — уровни не сверены: %s\n", levels.error.c_str());
}

void runSync(bool russian, Choice choice, bool withSave) {
    s_done = 0;
    s_total = 0;
    Outcome save, skins;
    std::string data = port::files::dataDir();
    makeDirs(data + "/Documents");
    makeDirs(data + "/skins");
    bool saveOk = !withSave || syncWhole("save", data + "/Documents", keepSave, choice, &save);
    bool skinsOk = !s_syncSkins || syncFiles("skins", data + "/skins", keepSkin, false, &skins, kSkinLimit);
    if (skins.down || skins.removed) s_skinsChanged = true;
    Outcome levels, prefs;
    makeDirs(data + "/levels");
    bool levelsOk = syncFiles("levels", data + "/levels", keepLevel, false, &levels);
    bool prefsOk = syncFiles("prefs", prefsDir(), keepPrefs, true, &prefs);
    if (prefs.down || prefs.removed) s_prefsChanged = true;
    std::string text = describe(save, skins, saveOk, skinsOk, russian, withSave);
    auto L = [russian](const char* ru, const char* en) { return std::string(russian ? ru : en); };
    if (!levelsOk) text += " " + L("Уровни не сверены: ", "Levels are not synced: ") + levels.error;
    else if (levels.down || levels.up || levels.removed)
        text += " " + L("Уровни: из аккаунта ", "Levels: from the account ") + std::to_string(levels.down) +
                L(", в аккаунт ", ", to the account ") + std::to_string(levels.up) + ".";
    if (!prefsOk) text += " " + L("Настройки не сверены: ", "Settings are not synced: ") + prefs.error;
    else if (prefs.down || prefs.removed) text += " " + L("Настройки из аккаунта применены.", "Settings from the account are applied.");
    if (withSave && saveOk && skinsOk && levelsOk && prefsOk && !save.pending) remove(unsyncedPath().c_str());
    fprintf(stderr, "medecomp: облако — %s (сохранение: ↓%d ↑%d ×%d, скины: ↓%d ↑%d ×%d, уровни: ↓%d ↑%d ×%d, настройки: ↓%d ↑%d)\n",
            text.c_str(), save.down, save.up, save.removed, skins.down, skins.up, skins.removed, levels.down, levels.up,
            levels.removed, prefs.down, prefs.up);
    std::lock_guard<std::mutex> lock(s_mu);
    s_status = text;
    s_conflict = save;
}

bool applyPrefs() {
    if (!s_prefsChanged.exchange(false)) return false;
    std::string dir = prefsDir(), text;
    Json j;
    if (readFile(dir + "/prefs.json", &text) && parseJson(text, &j) && j.type == Json::Object) {
        for (const char* key : kPortable) {
            const Json& v = j[key];
            if (v.type == Json::String) settings::set(key, v.str());
            else settings::erase(key);
        }
        settings::save();
    }
    std::string lut;
    if (readFile(dir + "/lut.png", &lut)) writeFile(port::files::dataDir() + "/lut.png", lut);
    else remove((port::files::dataDir() + "/lut.png").c_str());
    fprintf(stderr, "medecomp: облако — настройки из аккаунта применены\n");
    return true;
}

void launch(bool russian, Choice choice, bool withSave = true) {
    if (!allowed() || s_busy.exchange(true)) return;
    capture();
    applyPrefs();
    writePrefs();
    std::thread([russian, choice, withSave] {
        runSync(russian, choice, withSave);
        while (s_levelsAgain.exchange(false)) runLevels();
        s_busy = false;
    }).detach();
}

}

bool enabled() { return settings::get("cloud_sync", "1") != "0"; }

bool allowed() {
    for (const char* var : {"MEDECOMP_SCRIPT", "MEDECOMP_SHOT", "MEDECOMP_SKIN_PREVIEWS", "MEDECOMP_NO_CLOUD"})
        if (getenv(var)) return false;
    return enabled() && !token().empty();
}

void markPlaying() {
    if (!allowed()) return;
    makeDirs(port::files::dataDir() + "/cache");
    std::ofstream(unsyncedPath().c_str()) << time(nullptr) << "\n";
}

bool interrupted() {
    struct stat st;
    return allowed() && stat(unsyncedPath().c_str(), &st) == 0;
}

void progress(int* done, int* total) {
    *done = s_done;
    *total = s_total;
}

void setEnabled(bool on) {
    settings::set("cloud_sync", on ? "1" : "0");
    settings::save();
}

bool skinsEnabled() { return settings::get("cloud_skins", "1") != "0"; }

void setSkinsEnabled(bool on) {
    settings::set("cloud_skins", on ? "1" : "0");
    settings::save();
}

int customSkins() {
    int n = 0;
    std::string dir = port::files::dataDir() + "/skins";
    for (const std::string& name : port::files::list(dir))
        if (name.compare(0, 5, "skin-") == 0 && port::files::stat(dir + "/" + name).dir) n++;
    return n;
}

bool skinLimitActive() { return !token().empty() && enabled() && skinsEnabled(); }

void startSync(bool russian, bool afterGame, bool withSave) { launch(russian, afterGame ? Newer : Ask, withSave); }

void syncLevels() {
    if (!allowed()) return;
    if (s_busy.exchange(true)) {
        s_levelsAgain = true;
        return;
    }
    capture();
    std::thread([] {
        do {
            runLevels();
        } while (s_levelsAgain.exchange(false));
        s_busy = false;
    }).detach();
}

bool conflictPending(long long* localTime, long long* remoteTime) {
    std::lock_guard<std::mutex> lock(s_mu);
    if (!s_conflict.pending || s_busy) return false;
    *localTime = s_conflict.localTime;
    *remoteTime = s_conflict.remoteTime;
    return true;
}

void resolve(bool takeRemote, bool russian) {
    {
        std::lock_guard<std::mutex> lock(s_mu);
        s_conflict = Outcome();
    }
    launch(russian, takeRemote ? TakeRemote : TakeLocal);
}

bool busy() { return s_busy.load(); }

std::string status() {
    std::lock_guard<std::mutex> lock(s_mu);
    return s_status;
}

void waitSync(int timeoutMs) {
    for (int waited = 0; s_busy && waited < timeoutMs; waited += 50) std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void syncNow(bool russian) {
    waitSync(60000);
    applyDownloadedPrefs();
    if (!allowed() || s_busy.exchange(true)) return;
    capture();
    writePrefs();
    runSync(russian, Newer, true);
    while (s_levelsAgain.exchange(false)) runLevels();
    s_busy = false;
}

bool takeSkinsChanged() { return s_skinsChanged.exchange(false); }

bool applyDownloadedPrefs() { return !s_busy && applyPrefs(); }

}
}
}
