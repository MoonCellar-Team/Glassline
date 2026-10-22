#include "port/files.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#ifdef __linux__
#include <sys/mman.h>
#include <sys/syscall.h>
#endif

#include <map>
#include <mutex>

namespace {

struct Node {
    bool dir = false;
    uint64_t size = 0;
    uint64_t offset = 0;
    uint64_t packed = 0;
    uint32_t crc = 0;
    uint16_t method = 0;
};

struct Game {
    std::string source;
    std::string app;
    std::string bundle;
    FILE* zip = nullptr;
    std::mutex zipMu;
    std::map<std::string, Node> nodes;
    std::map<std::string, std::string> folded;
};

Game* s_game;
std::mutex s_openMu;

std::string lower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return s;
}

bool endsWith(const std::string& s, const char* tail) {
    size_t n = strlen(tail);
    return s.size() >= n && s.compare(s.size() - n, n, tail) == 0;
}

std::string normalize(const std::string& rawPath) {
#ifdef _WIN32
    std::string path = rawPath;
    for (char& c : path)
        if (c == '\\') c = '/';
#else
    const std::string& path = rawPath;
#endif
    std::vector<std::string> parts;
    size_t i = 0;
    while (i <= path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos) j = path.size();
        std::string part = path.substr(i, j - i);
        if (part == "..") {
            if (!parts.empty()) parts.pop_back();
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        i = j + 1;
    }
    std::string out = !path.empty() && path[0] == '/' ? "/" : "";
    for (size_t k = 0; k < parts.size(); k++) out += (k ? "/" : "") + parts[k];
    return out.empty() ? "." : out;
}

bool mkdirs(const std::string& dir) {
    for (size_t i = 1; i <= dir.size(); i++) {
        if (i < dir.size() && dir[i] != '/') continue;
        std::string part = dir.substr(0, i);
        if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) return false;
    }
    return true;
}

uint32_t le16(const unsigned char* p) { return p[0] | p[1] << 8; }
uint32_t le32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }
uint64_t le64(const unsigned char* p) { return le32(p) | uint64_t(le32(p + 4)) << 32; }

bool seek(FILE* f, uint64_t pos) {
#ifdef __GLIBC__
    return fseeko64(f, off64_t(pos), SEEK_SET) == 0;
#else
    return fseeko(f, off_t(pos), SEEK_SET) == 0;
#endif
}

bool readAt(FILE* f, uint64_t pos, void* buf, size_t n) { return seek(f, pos) && fread(buf, 1, n, f) == n; }

void addNode(Game* g, const std::string& rel, const Node& n) {
    for (size_t s = rel.find('/'); s != std::string::npos; s = rel.find('/', s + 1)) {
        Node d;
        d.dir = true;
        g->nodes.emplace(rel.substr(0, s), d);
        g->folded.emplace(lower(rel.substr(0, s)), rel.substr(0, s));
    }
    g->nodes[rel] = n;
    g->folded.emplace(lower(rel), rel);
}

bool scanZip(Game* g, std::string* error) {
    FILE* f = g->zip;
    uint64_t total = 0;
#ifdef __GLIBC__
    if (fseeko64(f, 0, SEEK_END) == 0) total = uint64_t(ftello64(f));
#else
    if (fseeko(f, 0, SEEK_END) == 0) total = uint64_t(ftello(f));
#endif
    size_t tail = size_t(total < 65557 ? total : 65557);
    std::vector<unsigned char> t(tail);
    if (tail < 22 || !readAt(f, total - tail, t.data(), tail)) {
        *error = "файл не читается или это не .ipa";
        return false;
    }
    long end = -1;
    for (long i = long(tail) - 22; i >= 0; i--)
        if (le32(&t[i]) == 0x06054b50) { end = i; break; }
    if (end < 0) {
        *error = "это не .ipa (не zip-архив)";
        return false;
    }
    uint64_t count = le16(&t[end + 10]), cdSize = le32(&t[end + 12]), cdPos = le32(&t[end + 16]);
    if (count == 0xFFFF || cdSize == 0xFFFFFFFF || cdPos == 0xFFFFFFFF) {
        unsigned char r[56];
        if (end < 20 || le32(&t[end - 20]) != 0x07064b50 || !readAt(f, le64(&t[end - 20 + 8]), r, 56) ||
            le32(r) != 0x06064b50) {
            *error = "повреждённый zip64";
            return false;
        }
        count = le64(r + 32);
        cdSize = le64(r + 40);
        cdPos = le64(r + 48);
    }
    std::vector<unsigned char> cd(static_cast<size_t>(cdSize));
    if (cdSize > total || !readAt(f, cdPos, cd.data(), cd.size())) {
        *error = "повреждённый .ipa: нет каталога файлов";
        return false;
    }
    std::vector<std::pair<std::string, Node>> all;
    size_t p = 0;
    for (uint64_t i = 0; i < count; i++) {
        if (p + 46 > cd.size() || le32(&cd[p]) != 0x02014b50) {
            *error = "повреждённый .ipa: каталог файлов обрывается";
            return false;
        }
        Node n;
        n.method = uint16_t(le16(&cd[p + 10]));
        n.crc = le32(&cd[p + 16]);
        n.packed = le32(&cd[p + 20]);
        n.size = le32(&cd[p + 24]);
        n.offset = le32(&cd[p + 42]);
        size_t nameLen = le16(&cd[p + 28]), extraLen = le16(&cd[p + 30]), commentLen = le16(&cd[p + 32]);
        if (p + 46 + nameLen + extraLen + commentLen > cd.size()) {
            *error = "повреждённый .ipa: каталог файлов обрывается";
            return false;
        }
        std::string name(reinterpret_cast<const char*>(&cd[p + 46]), nameLen);
        for (size_t q = p + 46 + nameLen, qe = q + extraLen; q + 4 <= qe;) {
            size_t id = le16(&cd[q]), len = le16(&cd[q + 2]), r = q + 4, re = q + 4 + len;
            if (id == 1) {
                if (n.size == 0xFFFFFFFF && r + 8 <= re) { n.size = le64(&cd[r]); r += 8; }
                if (n.packed == 0xFFFFFFFF && r + 8 <= re) { n.packed = le64(&cd[r]); r += 8; }
                if (n.offset == 0xFFFFFFFF && r + 8 <= re) n.offset = le64(&cd[r]);
            }
            q = re;
        }
        all.push_back({name, n});
        p += 46 + nameLen + extraLen + commentLen;
    }
    for (auto& a : all) {
        const std::string& name = a.first;
        if (name.compare(0, 8, "Payload/") != 0 || !endsWith(name, ".app/Info.plist")) continue;
        std::string app = name.substr(8, name.size() - 8 - 11);
        if (app.find('/') == std::string::npos) { g->app = app; break; }
    }
    if (g->app.empty()) {
        *error = "в .ipa нет приложения (Payload/*.app)";
        return false;
    }
    std::string prefix = "Payload/" + g->app + "/";
    Node root;
    root.dir = true;
    g->nodes[""] = root;
    for (auto& a : all) {
        if (a.first.compare(0, prefix.size(), prefix) != 0) continue;
        std::string rel = a.first.substr(prefix.size());
        bool dir = !rel.empty() && rel.back() == '/';
        if (dir) rel.pop_back();
        if (rel.empty()) continue;
        a.second.dir = dir;
        addNode(g, rel, a.second);
    }
    return true;
}

void scanDir(Game* g, const std::string& real, const std::string& rel) {
    DIR* d = opendir(real.c_str());
    if (!d) return;
    while (dirent* e = readdir(d)) {
        std::string name = e->d_name;
        if (name == "." || name == "..") continue;
        struct stat st;
        if (::stat((real + "/" + name).c_str(), &st) != 0) continue;
        Node n;
        n.dir = S_ISDIR(st.st_mode);
        n.size = uint64_t(st.st_size);
        std::string sub = rel.empty() ? name : rel + "/" + name;
        addNode(g, sub, n);
        if (n.dir) scanDir(g, real + "/" + name, sub);
    }
    closedir(d);
}

std::string findApp(const std::string& dir) {
    if (endsWith(dir, ".app")) return dir;
    for (const std::string& d : {dir, dir + "/Payload"}) {
        DIR* h = opendir(d.c_str());
        if (!h) continue;
        std::string found;
        while (dirent* e = readdir(h)) {
            std::string n = e->d_name;
            if (endsWith(n, ".app")) { found = d + "/" + n; break; }
        }
        closedir(h);
        if (!found.empty()) return found;
    }
    return std::string();
}

std::string exeDir() {
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    std::string dir = n > 0 ? std::string(exe, size_t(n)) : std::string(".");
    size_t slash = dir.rfind('/');
    return slash == std::string::npos ? std::string(".") : dir.substr(0, slash);
}

Game* game() {
    if (s_game) return s_game;
    std::string error;
    for (const char* var : {"MEDECOMP_GAME", "MEDECOMP_BUNDLE"})
        if (const char* v = getenv(var))
            if (*v && port::files::openGame(v, &error)) return s_game;
    std::string dir = exeDir();
    for (int i = 0; i < 8 && !s_game; i++) {
        std::string app = findApp(dir + "/orig");
        if (!app.empty() && port::files::openGame(app, &error)) break;
        size_t slash = dir.rfind('/');
        if (slash == std::string::npos || slash == 0) break;
        dir.resize(slash);
    }
    if (!s_game) {
        fprintf(stderr, "medecomp: игра не указана — путь к .ipa в MEDECOMP_GAME или в лаунчере\n");
        static Game empty;
        empty.bundle = port::files::dataDir() + "/mirrorsedge_ipad.app";
        empty.nodes[""].dir = true;
        s_game = &empty;
    }
    return s_game;
}

const Node* lookup(const std::string& path, std::string* rel) {
    Game* g = game();
    std::string p = normalize(path);
    if (p == g->bundle) {
        rel->clear();
    } else if (p.size() > g->bundle.size() && p.compare(0, g->bundle.size(), g->bundle) == 0 &&
               p[g->bundle.size()] == '/') {
        *rel = p.substr(g->bundle.size() + 1);
    } else {
        return nullptr;
    }
    auto it = g->nodes.find(*rel);
    if (it == g->nodes.end()) {
        auto f = g->folded.find(lower(*rel));
        if (f == g->folded.end()) return nullptr;
        *rel = f->second;
        it = g->nodes.find(*rel);
    }
    return &it->second;
}

bool inBundle(const std::string& path) {
    Game* g = game();
    std::string p = normalize(path);
    return p == g->bundle || (p.size() > g->bundle.size() && p.compare(0, g->bundle.size(), g->bundle) == 0 &&
                              p[g->bundle.size()] == '/');
}

bool readZip(Game* g, const Node& n, std::vector<unsigned char>* out) {
    std::vector<unsigned char> packed(size_t(n.packed));
    {
        std::lock_guard<std::mutex> lock(g->zipMu);
        unsigned char h[30];
        if (!readAt(g->zip, n.offset, h, 30) || le32(h) != 0x04034b50) return false;
        uint64_t data = n.offset + 30 + le16(h + 26) + le16(h + 28);
        if (!packed.empty() && !readAt(g->zip, data, packed.data(), packed.size())) return false;
    }
    if (n.method == 0) {
        out->swap(packed);
    } else if (n.method == 8) {
        out->assign(size_t(n.size), 0);
        z_stream z;
        memset(&z, 0, sizeof z);
        if (inflateInit2(&z, -MAX_WBITS) != Z_OK) return false;
        z.next_in = packed.data();
        z.avail_in = uInt(packed.size());
        z.next_out = out->data();
        z.avail_out = uInt(out->size());
        int r = inflate(&z, Z_FINISH);
        uint64_t got = z.total_out;
        inflateEnd(&z);
        if ((r != Z_STREAM_END && !(r == Z_BUF_ERROR && out->empty())) || got != n.size) return false;
    } else {
        return false;
    }
    return out->size() == n.size && crc32(0, out->data(), uInt(out->size())) == n.crc;
}

FILE* toMemoryFile(const std::vector<unsigned char>& data) {
#if defined(__ANDROID__) && __ANDROID_API__ < 30
    int fd = int(syscall(__NR_memfd_create, "medecomp", 1u ));
#elif defined(__linux__)
    int fd = memfd_create("medecomp", MFD_CLOEXEC);
#endif
#ifdef __linux__
    if (fd >= 0) {
        size_t done = 0;
        while (done < data.size()) {
            ssize_t n = write(fd, data.data() + done, data.size() - done);
            if (n <= 0) break;
            done += size_t(n);
        }
        if (done == data.size() && lseek(fd, 0, SEEK_SET) == 0)
            if (FILE* f = fdopen(fd, "rb")) return f;
        close(fd);
    }
#endif
    FILE* f = tmpfile();
    if (!f) return nullptr;
    if (!data.empty() && fwrite(data.data(), 1, data.size(), f) != data.size()) {
        fclose(f);
        return nullptr;
    }
    rewind(f);
    return f;
}

}

namespace port {
namespace files {

const std::string& dataDir() {
    static const std::string dir = [] {
        std::string d;
        const char* own = getenv("MEDECOMP_DATA");
        const char* xdg = getenv("XDG_DATA_HOME");
        const char* home = getenv("HOME");
#ifdef _WIN32
        const char* appdata = getenv("APPDATA");
#endif
        if (own && *own) d = own;
#ifdef _WIN32
        else if (appdata && *appdata) d = std::string(appdata) + "/mirrors-edge";
#endif
        else if (xdg && *xdg == '/') d = std::string(xdg) + "/mirrors-edge";
        else if (home && *home) d = std::string(home) + "/.local/share/mirrors-edge";
        else d = "mirrors-edge";
        d = normalize(d);
        if (!mkdirs(d)) fprintf(stderr, "medecomp: не создать папку игрока %s\n", d.c_str());
        return d;
    }();
    return dir;
}

bool openGame(const std::string& source, std::string* error) {
    std::string dummy;
    if (!error) error = &dummy;
    std::lock_guard<std::mutex> lock(s_openMu);
    struct stat st;
    if (::stat(source.c_str(), &st) != 0) {
        *error = "файл не найден";
        return false;
    }
    Game* g = new Game();
    g->source = normalize(source);
    if (S_ISDIR(st.st_mode)) {
        std::string app = findApp(g->source);
        if (app.empty() || ::stat((app + "/Info.plist").c_str(), &st) != 0) {
            *error = "в папке нет приложения (.app с Info.plist)";
            delete g;
            return false;
        }
        g->source = app;
        g->app = app.substr(app.rfind('/') + 1);
        g->nodes[""].dir = true;
        scanDir(g, app, "");
    } else {
        g->zip = fopen(source.c_str(), "rb");
        if (!g->zip || !scanZip(g, error)) {
            if (g->zip) fclose(g->zip);
            if (error->empty()) *error = "файл не читается";
            delete g;
            return false;
        }
    }
    g->bundle = dataDir() + "/" + g->app;
    s_game = g;
    return true;
}

const std::string& gameSource() { return game()->source; }

const std::string& bundle() { return game()->bundle; }

Info stat(const std::string& path) {
    Info info;
    std::string rel;
    if (inBundle(path)) {
        if (const Node* n = lookup(path, &rel)) {
            info.exists = true;
            info.dir = n->dir;
            info.size = n->size;
        }
        return info;
    }
    struct stat st;
    if (::stat(path.c_str(), &st) == 0) {
        info.exists = true;
        info.dir = S_ISDIR(st.st_mode);
        info.size = uint64_t(st.st_size);
    }
    return info;
}

std::vector<std::string> list(const std::string& dir) {
    std::vector<std::string> out;
    std::string rel;
    if (inBundle(dir)) {
        const Node* n = lookup(dir, &rel);
        if (!n || !n->dir) return out;
        Game* g = game();
        std::string prefix = rel.empty() ? "" : rel + "/";
        for (auto it = g->nodes.lower_bound(prefix); it != g->nodes.end(); ++it) {
            if (it->first.compare(0, prefix.size(), prefix) != 0) break;
            std::string name = it->first.substr(prefix.size());
            if (!name.empty() && name.find('/') == std::string::npos) out.push_back(name);
        }
        return out;
    }
    if (DIR* d = opendir(dir.c_str())) {
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name;
            if (n != "." && n != "..") out.push_back(n);
        }
        closedir(d);
    }
    return out;
}

bool read(const std::string& path, std::vector<unsigned char>* out) {
    out->clear();
    std::string rel;
    std::string real = path;
    if (inBundle(path)) {
        const Node* n = lookup(path, &rel);
        if (!n || n->dir) return false;
        Game* g = game();
        if (g->zip) return readZip(g, *n, out);
        real = g->source + "/" + rel;
    }
    FILE* f = fopen(real.c_str(), "rb");
    if (!f) return false;
    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out->insert(out->end(), buf, buf + n);
    bool ok = !ferror(f);
    fclose(f);
    return ok;
}

FILE* open(const std::string& path) {
    std::string rel;
    if (!inBundle(path)) return fopen(path.c_str(), "rb");
    const Node* n = lookup(path, &rel);
    if (!n || n->dir) return nullptr;
    Game* g = game();
    if (!g->zip) return fopen((g->source + "/" + rel).c_str(), "rb");
    std::vector<unsigned char> data;
    if (!readZip(g, *n, &data)) {
        fprintf(stderr, "medecomp: не прочитать %s из %s\n", rel.c_str(), g->source.c_str());
        return nullptr;
    }
    return toMemoryFile(data);
}

FILE* memoryFile(const std::vector<unsigned char>& data) { return toMemoryFile(data); }

}
}
