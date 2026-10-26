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
#include <random>
#include <set>
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
    int version = 1;
    long long revision = -1;
    std::string created;
    Files files;
};

struct Outcome {
    int down = 0, up = 0, removed = 0;
    int skipped = 0;
    int released = 0;
    int keptHere = 0;
    int renamed = 0;
    bool reset = false;
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

const char* const kHereMark = ".local";
const char* const kReleased = "local-only.json";
const size_t kReleasedMax = 64;

bool keepSkin(const std::string& path) {
    size_t slash = path.find('/');
    return path.compare(0, 5, "skin-") == 0 && slash != std::string::npos &&
           path.find('/', slash + 1) == std::string::npos && !partial(path) && path.compare(slash + 1, std::string::npos, kHereMark) != 0;
}

std::set<std::string> s_hereFolders;

bool keepSyncedSkin(const std::string& path) {
    return keepSkin(path) && !s_hereFolders.count(path.substr(0, path.find('/')));
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

Manifest loadManifest(const std::string& bucket, const std::string& server = s_server, const std::string& user = s_user) {
    Manifest m;
    std::string text;
    Json j;
    if (!readFile(manifestPath(bucket), &text) || !parseJson(text, &j) || j.type != Json::Object) return m;
    if (j["server"].str() != server || j["user"].str() != user) return m;
    m.valid = true;
    m.version = int(j["v"].integer(1));
    m.revision = j["revision"].integer(-1);
    m.created = j["created"].str();
    for (const auto& f : j["files"].fields) {
        Entry e;
        e.sha = f.second["sha"].str();
        e.size = f.second["size"].integer();
        e.time = f.second["time"].integer();
        m.files[f.first] = e;
    }
    return m;
}

void saveManifest(const std::string& bucket, long long revision, const std::string& created, const Files& local) {
    std::ostringstream s;
    s << "{\"v\":2,\"server\":" << jsonString(s_server) << ",\"user\":" << jsonString(s_user) << ",\"revision\":" << revision
      << ",\"created\":" << jsonString(created) << ",\"files\":{";
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

bool listRemote(const std::string& bucket, Files* out, long long* revision, std::string* created, long long* maxFile,
                std::string* error) {
    Response r = request("GET", bucketUrl(bucket), std::string(), authHeaders());
    Json j;
    if (r.status != 200 || !parseJson(r.body, &j)) return *error = errorOf(r.status, r.body, r.error), false;
    *revision = j["revision"].integer();
    *created = j["createdAt"].str();
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
             std::string* error, int* status = nullptr) {
    slow();
    std::vector<std::string> headers = authHeaders();
    headers.push_back("Content-Type: application/octet-stream");
    Response r = request("PUT", bucketUrl(bucket) + "/file?path=" + path, data, headers, 120000);
    if (status) *status = r.status;
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
            long long maxFile, long long* revision, Outcome* o, Files* sent = nullptr) {
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
        if (sent) (*sent)[f.first] = f.second;
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

void createdAfterWrites(const std::string& bucket, long long revision, std::string* created) {
    if (!created->empty() || revision <= 0) return;
    Files files;
    long long again = 0, maxFile = 0;
    std::string error;
    listRemote(bucket, &files, &again, created, &maxFile, &error);
}

void storageReset(const std::string& bucket, Manifest* m, const Files& remote, long long revision, const std::string& created,
                  Outcome* o) {
    bool other = !m->created.empty() && created != m->created, behind = m->revision >= 0 && revision <= m->revision;
    if (!m->valid || !(other || behind)) return;
    int lost = 0;
    for (const auto& f : m->files) lost += !remote.count(f.first);
    if (lost == 0) return;
    fprintf(stderr, "medecomp: облако — хранилище «%s» в аккаунте сброшено (%s; пропало файлов: %d): "
                    "здесь ничего не удаляется, своё уходит заново\n",
            bucket.c_str(),
            other ? ("корзина создана " + (created.empty() ? std::string("заново, ещё пуста") : created) + ", в слепке " + m->created).c_str()
                  : ("ревизия " + std::to_string(revision) + ", в слепке " + std::to_string(m->revision)).c_str(),
            lost);
    m->files.clear();
    o->reset = true;
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
    std::string created;
    if (!listRemote(bucket, &remote, &revision, &created, &maxFile, &o->error)) return false;
    storageReset(bucket, &m, remote, revision, created, o);
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
    createdAfterWrites(bucket, revision, &created);
    saveManifest(bucket, revision, created, local);
    return true;
}

std::string folderOf(const std::string& path) {
    size_t slash = path.find('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::set<std::string> foldersOf(const Files& files) {
    std::set<std::string> out;
    for (const auto& f : files)
        if (!folderOf(f.first).empty()) out.insert(folderOf(f.first));
    return out;
}

void eraseFolder(Files* files, const std::string& folder) {
    for (auto it = files->begin(); it != files->end();) {
        if (folderOf(it->first) == folder) it = files->erase(it);
        else ++it;
    }
}

Files inBoth(const Files& local, const Files& remote) {
    Files out;
    for (const auto& f : local) {
        auto r = remote.find(f.first);
        if (r != remote.end() && r->second.sha == f.second.sha) out[f.first] = f.second;
    }
    return out;
}

void forgetUnsent(Manifest* m, const Files& remote) {
    if (!m->valid || m->version >= 2) return;
    for (auto it = m->files.begin(); it != m->files.end();) {
        if (remote.count(it->first)) ++it;
        else it = m->files.erase(it);
    }
}

struct Plan {
    Files toDown, toUp, dropLocal, dropRemote;
};

Plan plan(const Files& local, const Files& remote, const Manifest& m, bool firstFromAccount,
          const std::set<std::string>& skip = std::set<std::string>()) {
    Plan p;
    std::map<std::string, bool> paths;
    for (const auto& f : local) paths[f.first] = true;
    for (const auto& f : remote) paths[f.first] = true;
    for (const auto& f : m.files) paths[f.first] = true;
    for (const auto& path : paths) {
        if (skip.count(folderOf(path.first))) continue;
        auto l = local.find(path.first), r = remote.find(path.first), b = m.files.find(path.first);
        std::string ls = l == local.end() ? "" : l->second.sha, rs = r == remote.end() ? "" : r->second.sha,
                    bs = b == m.files.end() ? "" : b->second.sha;
        if (ls == rs) continue;
        bool localSame = ls == bs, remoteSame = rs == bs;
        bool takeRemote = localSame ? true : remoteSame ? false
                                                         : (r != remote.end() ? r->second.time : 0) > (l != local.end() ? l->second.time : 0);
        if (!m.valid && firstFromAccount && r != remote.end()) takeRemote = true;
        if (takeRemote) {
            if (r != remote.end()) p.toDown[path.first] = r->second;
            else p.dropLocal[path.first] = l->second;
        } else {
            if (l != local.end()) p.toUp[path.first] = l->second;
            else p.dropRemote[path.first] = r->second;
        }
    }
    return p;
}

void removeLocal(const std::string& dir, const Files& drop, Outcome* o) {
    for (const auto& f : drop)
        if (remove((dir + "/" + f.first).c_str()) == 0) o->removed++;
    for (const auto& f : drop) {
        std::string folder = folderOf(f.first);
        if (!folder.empty() && port::files::list(dir + "/" + folder).empty()) remove((dir + "/" + folder).c_str());
    }
}

bool syncFiles(const std::string& bucket, const std::string& dir, bool (*keep)(const std::string&), bool firstFromAccount,
               Outcome* o) {
    Manifest m = loadManifest(bucket);
    Files local, remote, listed;
    scan(dir, std::string(), 0, keep, m.files, &local);
    long long revision = 0, maxFile = 0;
    std::string created;
    if (!listRemote(bucket, &listed, &revision, &created, &maxFile, &o->error)) return false;
    for (const auto& f : listed)
        if (keep(f.first)) remote.insert(f);
    storageReset(bucket, &m, remote, revision, created, o);
    forgetUnsent(&m, remote);
    Plan p = plan(local, remote, m, firstFromAccount);
    s_total += int(p.toDown.size() + p.toUp.size());
    if (!download(bucket, dir, Files(), p.toDown, false, o)) return false;
    removeLocal(dir, p.dropLocal, o);
    for (const auto& f : p.dropRemote) {
        if (!deleteFile(bucket, f.first, &revision, &o->error)) return false;
        remote.erase(f.first);
        o->removed++;
    }
    Files sent;
    if (!upload(bucket, dir, p.toUp, Files(), false, maxFile, &revision, o, &sent)) return false;
    for (const auto& f : sent) remote[f.first] = f.second;
    local.clear();
    scan(dir, std::string(), 0, keep, Files(), &local);
    createdAfterWrites(bucket, revision, &created);
    saveManifest(bucket, revision, created, inBoth(local, remote));
    return true;
}

std::vector<std::string> skinFolders(const std::string& dir) {
    std::vector<std::string> out;
    for (const std::string& name : port::files::list(dir))
        if (name.compare(0, 5, "skin-") == 0 && port::files::stat(dir + "/" + name).dir) out.push_back(name);
    return out;
}

bool marked(const std::string& dir, const std::string& folder) {
    struct stat st;
    return stat((dir + "/" + folder + "/" + kHereMark).c_str(), &st) == 0;
}

void mark(const std::string& dir, const std::string& folder, bool here) {
    std::string path = dir + "/" + folder + "/" + kHereMark;
    if (here) std::ofstream(path.c_str()) << "Только здесь: в облако MoonCellar не отправляется / Local only: not synced to the MoonCellar cloud\n";
    else remove(path.c_str());
}

Files folderFiles(const std::string& dir, const std::string& folder) {
    Files out;
    for (const std::string& name : port::files::list(dir + "/" + folder)) {
        std::string path = folder + "/" + name, data;
        struct stat st;
        if (!keepSkin(path) || !validPath(path) || stat((dir + "/" + path).c_str(), &st) != 0 || S_ISDIR(st.st_mode) ||
            !readFile(dir + "/" + path, &data))
            continue;
        Entry e;
        e.sha = sha256(data);
        e.size = (long long)st.st_size;
        e.time = (long long)st.st_mtime;
        out[path] = e;
    }
    return out;
}

std::mutex s_idMu;

std::string makeSkinId(const std::string& dir, const std::set<std::string>& taken) {
    std::lock_guard<std::mutex> lock(s_idMu);
    static std::mt19937 rng(std::random_device()() ^ unsigned(time(nullptr)) ^ unsigned(clock()));
    time_t now = time(nullptr);
    struct tm t;
#ifdef _WIN32
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    char when[32];
    strftime(when, sizeof when, "%Y%m%d%H%M%S", &t);
    const char* const kLetters = "abcdefghijklmnopqrstuvwxyz0123456789";
    while (true) {
        std::string id = std::string("skin-") + when + "-";
        for (int i = 0; i < 3; i++) id += kLetters[rng() % 36];
        struct stat st;
        if (!taken.count(id) && stat((dir + "/" + id).c_str(), &st) != 0) return id;
    }
}

std::map<std::string, long long> parseReleased(const std::string& text) {
    std::map<std::string, long long> out;
    Json j;
    if (parseJson(text, &j) && j.type == Json::Object)
        for (const auto& f : j.fields)
            if (f.first.compare(0, 5, "skin-") == 0 && validPath(f.first)) out[f.first] = f.second.integer();
    return out;
}

bool putReleased(std::map<std::string, long long>* list, bool* exists, long long* revision, std::string* error) {
    while (list->size() > kReleasedMax) {
        auto oldest = list->begin();
        for (auto it = list->begin(); it != list->end(); ++it)
            if (it->second < oldest->second) oldest = it;
        list->erase(oldest);
    }
    if (list->empty()) {
        if (*exists && !deleteFile("skins", kReleased, revision, error)) return false;
        *exists = false;
        return true;
    }
    std::ostringstream s;
    s << "{";
    for (auto it = list->begin(); it != list->end(); ++it) s << (it == list->begin() ? "" : ",") << jsonString(it->first) << ":" << it->second;
    s << "}\n";
    if (!putFile("skins", kReleased, s.str(), revision, error)) return false;
    *exists = true;
    return true;
}

std::mutex s_renameMu;
std::vector<std::pair<std::string, std::string>> s_renames;
std::atomic<int> s_skinsSaved(0);

void leaveHere(const std::string& dir, const std::string& folder, Files* local, Files* toUp) {
    mark(dir, folder, true);
    s_hereFolders.insert(folder);
    eraseFolder(local, folder);
    if (toUp) eraseFolder(toUp, folder);
}

bool syncSkinFolders(const std::string& dir, Outcome* o) {
    const std::string bucket = "skins";
    Manifest m = loadManifest(bucket);
    s_hereFolders.clear();
    for (const std::string& f : skinFolders(dir))
        if (marked(dir, f)) s_hereFolders.insert(f);
    Files local, remote, listed;
    scan(dir, std::string(), 0, keepSyncedSkin, m.files, &local);
    long long revision = 0, maxFile = 0;
    std::string created;
    if (!listRemote(bucket, &listed, &revision, &created, &maxFile, &o->error)) return false;
    bool releasedThere = false;
    for (const auto& f : listed) {
        if (f.first == kReleased) releasedThere = true;
        else if (keepSkin(f.first)) remote.insert(f);
    }
    storageReset(bucket, &m, remote, revision, created, o);
    forgetUnsent(&m, remote);
    std::map<std::string, long long> released;
    if (releasedThere) {
        std::string text;
        if (!fetchFile(bucket, kReleased, &text, &o->error)) return false;
        released = parseReleased(text);
    }
    std::map<std::string, long long> written = released;
    std::set<std::string> base = foldersOf(m.files);

    std::vector<std::string> release;
    for (const std::string& f : s_hereFolders)
        if (base.count(f)) release.push_back(f);
    if (!release.empty()) {
        for (const std::string& f : release) released[f] = (long long)time(nullptr);
        if (!putReleased(&released, &releasedThere, &revision, &o->error)) return false;
        written = released;
        for (const std::string& f : release) {
            for (auto it = remote.begin(); it != remote.end();) {
                if (folderOf(it->first) != f) {
                    ++it;
                    continue;
                }
                if (!deleteFile(bucket, it->first, &revision, &o->error)) return false;
                it = remote.erase(it);
            }
            o->released++;
        }
    }
    for (const std::string& f : base) {
        if (!released.count(f) || s_hereFolders.count(f) || !port::files::stat(dir + "/" + f).dir) continue;
        leaveHere(dir, f, &local, nullptr);
        o->keptHere++;
    }
    std::set<std::string> there = foldersOf(remote);
    for (const std::string& f : there) {
        if (base.count(f) || !port::files::stat(dir + "/" + f).dir) continue;
        Files mine = folderFiles(dir, f), theirs;
        for (const auto& r : remote)
            if (folderOf(r.first) == f) theirs.insert(r);
        bool same = mine.empty() || mine.size() == theirs.size();
        for (const auto& x : mine) {
            auto t = theirs.find(x.first);
            same = same && t != theirs.end() && t->second.sha == x.second.sha;
        }
        if (same) {
            if (s_hereFolders.erase(f)) mark(dir, f, false);
            for (const auto& x : mine) local[x.first] = x.second;
            continue;
        }
        std::string id = makeSkinId(dir, there);
        if (rename((dir + "/" + f).c_str(), (dir + "/" + id).c_str()) != 0) return o->error = "не переименовать " + dir + "/" + f, false;
        if (s_hereFolders.erase(f)) s_hereFolders.insert(id);
        Files moved;
        for (const auto& x : local)
            if (folderOf(x.first) == f) moved[id + x.first.substr(f.size())] = x.second;
        eraseFolder(&local, f);
        local.insert(moved.begin(), moved.end());
        {
            std::lock_guard<std::mutex> lock(s_renameMu);
            s_renames.push_back({f, id});
        }
        fprintf(stderr, "medecomp: облако — скин %s переименован в %s: в аккаунте под этим именем другой\n", f.c_str(), id.c_str());
        o->renamed++;
    }

    Plan p = plan(local, remote, m, false, s_hereFolders);
    std::set<std::string> held;
    for (const auto& f : remote)
        if (!p.dropRemote.count(f.first)) held.insert(folderOf(f.first));
    std::map<std::string, long long> born;
    for (const auto& f : p.toUp) {
        std::string folder = folderOf(f.first);
        if (held.count(folder)) continue;
        auto it = born.find(folder);
        if (it == born.end() || f.second.time < it->second) born[folder] = f.second.time;
    }
    std::vector<std::pair<long long, std::string>> order;
    for (const auto& f : born) order.push_back({f.second, f.first});
    std::sort(order.begin(), order.end(), [](const std::pair<long long, std::string>& a, const std::pair<long long, std::string>& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second.size() != b.second.size() ? a.second.size() < b.second.size() : a.second < b.second;
    });
    for (const auto& f : order) {
        if (int(held.size()) < kSkinLimit) {
            held.insert(f.second);
            continue;
        }
        leaveHere(dir, f.second, &local, &p.toUp);
        o->skipped++;
    }

    s_total += int(p.toDown.size() + p.toUp.size());
    if (!download(bucket, dir, Files(), p.toDown, false, o)) return false;
    removeLocal(dir, p.dropLocal, o);
    for (const auto& f : p.dropRemote) {
        if (!deleteFile(bucket, f.first, &revision, &o->error)) return false;
        remote.erase(f.first);
        o->removed++;
    }
    there = foldersOf(remote);
    for (const std::string& f : foldersOf(p.dropRemote))
        if (!there.count(f)) released.erase(f);
    std::string refused;
    for (const auto& f : p.toUp) {
        std::string folder = folderOf(f.first);
        if (folder == refused) continue;
        if (f.second.size > maxFile) {
            fprintf(stderr, "medecomp: облако — %s больше %lld Б, в аккаунт не уходит\n", f.first.c_str(), maxFile);
            continue;
        }
        std::string data;
        if (!readFile(dir + "/" + f.first, &data)) continue;
        int status = 0;
        if (!putFile(bucket, f.first, data, &revision, &o->error, &status)) {
            if (status != 422 || foldersOf(remote).count(folder)) return false;
            fprintf(stderr, "medecomp: облако — скин %s не принят (%s): только здесь\n", folder.c_str(), o->error.c_str());
            o->error.clear();
            refused = folder;
            mark(dir, folder, true);
            s_hereFolders.insert(folder);
            o->skipped++;
            continue;
        }
        remote[f.first] = f.second;
        o->up++;
        s_done++;
    }
    for (const std::string& f : foldersOf(remote)) released.erase(f);
    if (released != written && !putReleased(&released, &releasedThere, &revision, &o->error)) return false;
    local.clear();
    scan(dir, std::string(), 0, keepSyncedSkin, Files(), &local);
    createdAfterWrites(bucket, revision, &created);
    saveManifest(bucket, revision, created, inBoth(local, remote));
    s_skinsSaved++;
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
std::atomic<bool> s_skinsAgain(false), s_skinsRussian(false);
Outcome s_conflict;
std::atomic<bool> s_busy(false), s_skinsChanged(false);
std::string s_status, s_skinsNotice;

std::string skinsNoticeOf(const Outcome& skins, bool ok, bool russian) {
    auto L = [russian](const char* ru, const char* en) { return std::string(russian ? ru : en); };
    if (!ok) return L("Скины не сверены с облаком: ", "Skins are not synced with the cloud: ") + skins.error;
    std::string out;
    if (skins.reset)
        out += L("Скины в облаке пропали (хранилище аккаунта сброшено на сервере) — здесь ничего не удалено, свои отправлены заново.",
                 "The skins in the cloud were gone (the account storage was reset on the server) — nothing was removed here, "
                 "yours are sent again.");
    if (skins.skipped)
        out += (out.empty() ? "" : " ") + L("В облаке уже ", "The cloud already holds ") + std::to_string(kSkinLimit) + L(" скинов — ещё ", " skins — ") +
               std::to_string(skins.skipped) +
               L(" оставлено только здесь: «В облако» — когда освободится место («Убрать из облака» у другого).",
                 " more stay here only: Send to cloud once there is room (Remove from cloud on another one).");
    if (skins.keptHere)
        out += (out.empty() ? "" : " ") + L("Убраны из облака на другом компьютере, здесь остались (только здесь): ",
                                              "Removed from the cloud on another computer, kept here (here only): ") +
               std::to_string(skins.keptHere) + ".";
    if (skins.renamed)
        out += (out.empty() ? "" : " ") +
               L("Здешние скины назывались так же, как другие скины в облаке, — переименованы, остались оба: ",
                 "Skins here had the same names as other skins in the cloud — renamed, both are kept: ") +
               std::to_string(skins.renamed) + ".";
    return out;
}

bool skinsMoved(const Outcome& o) {
    return o.down || o.up || o.removed || o.skipped || o.released || o.keptHere || o.renamed || o.reset;
}

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
    if (skinsOk && (skins.down || skins.up || skins.removed))
        out += " " + L("Скины: из аккаунта ", "Skins: from the account ") + std::to_string(skins.down) + L(", в аккаунт ", ", to the account ") +
               std::to_string(skins.up) + (skins.removed ? L(", удалено ", ", removed ") + std::to_string(skins.removed) : std::string()) + ".";
    if (skinsOk && skins.released)
        out += " " + L("Убрано из облака скинов (здесь остались): ", "Skins removed from the cloud (kept here): ") +
               std::to_string(skins.released) + ".";
    std::string notice = skinsNoticeOf(skins, skinsOk, russian);
    if (!notice.empty()) out += " " + notice;
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

void runSkins(bool russian) {
    if (!s_syncSkins) return;
    Outcome skins;
    std::string dir = port::files::dataDir() + "/skins";
    makeDirs(dir);
    bool ok = syncSkinFolders(dir, &skins);
    if (!ok || skinsMoved(skins)) s_skinsChanged = true;
    fprintf(stderr, "medecomp: облако — скины: ↓%d ↑%d ×%d, убрано %d, только здесь +%d, переименовано %d%s%s\n", skins.down,
            skins.up, skins.removed, skins.released, skins.skipped + skins.keptHere, skins.renamed, ok ? "" : " — не сверены: ",
            skins.error.c_str());
    std::lock_guard<std::mutex> lock(s_mu);
    s_skinsNotice = skinsNoticeOf(skins, ok, russian);
}

void runAgain() {
    for (bool more = true; more;) {
        more = false;
        if (s_levelsAgain.exchange(false)) runLevels(), more = true;
        if (s_skinsAgain.exchange(false)) runSkins(s_skinsRussian), more = true;
    }
}

void runSync(bool russian, Choice choice, bool withSave) {
    s_done = 0;
    s_total = 0;
    Outcome save, skins;
    std::string data = port::files::dataDir();
    makeDirs(data + "/Documents");
    makeDirs(data + "/skins");
    bool saveOk = !withSave || syncWhole("save", data + "/Documents", keepSave, choice, &save);
    bool skinsOk = !s_syncSkins || syncSkinFolders(data + "/skins", &skins);
    if (!skinsOk || skinsMoved(skins)) s_skinsChanged = true;
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
    std::string lost;
    for (const auto& b : {std::make_pair(&save, L("сохранение", "save")), std::make_pair(&levels, L("уровни", "levels")),
                          std::make_pair(&prefs, L("настройки", "settings"))})
        if (b.first->reset) lost += (lost.empty() ? "" : ", ") + b.second;
    if (!lost.empty())
        text += " " + L("В облаке пропали файлы аккаунта (хранилище сброшено на сервере): ", "The account's files were gone from the cloud (the storage was reset on the server): ") +
                lost + L(" — здесь ничего не удалено, отправлено заново.", " — nothing was removed here, sent again.");
    if (withSave && saveOk && skinsOk && levelsOk && prefsOk && !save.pending) remove(unsyncedPath().c_str());
    fprintf(stderr, "medecomp: облако — %s (сохранение: ↓%d ↑%d ×%d, скины: ↓%d ↑%d ×%d, уровни: ↓%d ↑%d ×%d, настройки: ↓%d ↑%d)\n",
            text.c_str(), save.down, save.up, save.removed, skins.down, skins.up, skins.removed, levels.down, levels.up,
            levels.removed, prefs.down, prefs.up);
    std::lock_guard<std::mutex> lock(s_mu);
    s_status = text;
    s_conflict = save;
    if (s_syncSkins) s_skinsNotice = skinsNoticeOf(skins, skinsOk, russian);
}

void applyRenames() {
    std::vector<std::pair<std::string, std::string>> renames;
    {
        std::lock_guard<std::mutex> lock(s_renameMu);
        renames.swap(s_renames);
    }
    std::string skin = settings::get("skin", "");
    bool changed = false;
    for (const auto& r : renames)
        if (skin == r.first) skin = r.second, changed = true;
    if (!changed) return;
    settings::set("skin", skin);
    settings::save();
}

bool applyPrefs() {
    applyRenames();
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
        runAgain();
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

bool skinsActive() { return !token().empty() && enabled() && skinsEnabled(); }

namespace {

std::string skinsDir() { return port::files::dataDir() + "/skins"; }

const Manifest& skinsManifest() {
    static Manifest m;
    static int seen = -1;
    static std::string seenFor;
    std::string who = serverUrl() + "\n" + userName();
    if (seen != s_skinsSaved || seenFor != who) {
        seen = s_skinsSaved;
        seenFor = who;
        m = loadManifest("skins", serverUrl(), userName());
    }
    return m;
}

}

SkinPlace skinPlace(const std::string& id) {
    if (!skinsActive()) return SkinPlace::Off;
    std::string dir = skinsDir();
    if (marked(dir, id)) return SkinPlace::Here;
    Files saved;
    for (const auto& f : skinsManifest().files)
        if (folderOf(f.first) == id) saved.insert(f);
    if (saved.empty()) return SkinPlace::New;
    size_t files = 0, same = 0;
    for (const std::string& name : port::files::list(dir + "/" + id)) {
        std::string path = id + "/" + name;
        struct stat st;
        if (!keepSkin(path) || stat((dir + "/" + path).c_str(), &st) != 0 || S_ISDIR(st.st_mode)) continue;
        files++;
        auto s = saved.find(path);
        if (s != saved.end() && s->second.size == (long long)st.st_size && s->second.time == (long long)st.st_mtime) same++;
    }
    return files == same && files == saved.size() ? SkinPlace::Saved : SkinPlace::Changed;
}

int skinsForCloud() {
    std::string dir = skinsDir();
    int n = 0;
    for (const std::string& f : skinFolders(dir))
        if (!marked(dir, f)) n++;
    return n;
}

bool placeNewSkin(const std::string& id) {
    if (!skinsActive() || skinsForCloud() <= kSkinLimit) return false;
    mark(skinsDir(), id, true);
    return true;
}

bool sendSkin(const std::string& id, bool russian) {
    if (skinsForCloud() >= kSkinLimit) return false;
    mark(skinsDir(), id, false);
    syncSkins(russian);
    return true;
}

void keepSkinHere(const std::string& id, bool russian) {
    mark(skinsDir(), id, true);
    syncSkins(russian);
}

std::string newSkinId() { return makeSkinId(skinsDir(), std::set<std::string>()); }

void syncSkins(bool russian) {
    if (!allowed() || !skinsEnabled()) return;
    s_skinsRussian = russian;
    if (s_busy.exchange(true)) {
        s_skinsAgain = true;
        return;
    }
    capture();
    std::thread([] {
        runSkins(s_skinsRussian);
        runAgain();
        s_busy = false;
    }).detach();
}

std::string skinsNotice() {
    std::lock_guard<std::mutex> lock(s_mu);
    return s_skinsNotice;
}

void startSync(bool russian, bool afterGame, bool withSave) { launch(russian, afterGame ? Newer : Ask, withSave); }

void syncLevels() {
    if (!allowed()) return;
    if (s_busy.exchange(true)) {
        s_levelsAgain = true;
        return;
    }
    capture();
    std::thread([] {
        runLevels();
        runAgain();
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
    runAgain();
    s_busy = false;
}

bool takeSkinsChanged() { return s_skinsChanged.exchange(false); }

bool applyDownloadedPrefs() { return !s_busy && applyPrefs(); }

}
}
}
