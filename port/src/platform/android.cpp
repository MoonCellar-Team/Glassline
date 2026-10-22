#ifdef __ANDROID__

#include <SDL.h>
#include <SDL_system.h>
#include <android/log.h>
#include <jni.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <thread>

#include "port/files.h"
#include "port/platform.h"
#include "settings.h"

namespace port {
namespace platform {

namespace {

void logToLogcat(const std::string& path) {
    int fds[2];
    if (pipe(fds) != 0) return;
    dup2(fds[1], 1);
    dup2(fds[1], 2);
    close(fds[1]);
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    FILE* file = fopen(path.c_str(), "w");
    std::thread([fd = fds[0], file] {
        char buf[4096];
        std::string line;
        ssize_t n;
        while ((n = read(fd, buf, sizeof buf)) > 0) {
            line.append(buf, size_t(n));
            size_t nl;
            while ((nl = line.find('\n')) != std::string::npos) {
                std::string one = line.substr(0, nl);
                __android_log_write(ANDROID_LOG_INFO, "medecomp", one.c_str());
                if (file) {
                    fputs(one.c_str(), file);
                    fputc('\n', file);
                    fflush(file);
                }
                line.erase(0, nl + 1);
            }
        }
    }).detach();
    setenv("MEDECOMP_LOG_FILE", path.c_str(), 1);
}

void makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++)
        if (i == path.size() || path[i] == '/') mkdir(path.substr(0, i).c_str(), 0755);
}

std::string readAsset(const char* name) {
    std::string out;
    SDL_RWops* in = SDL_RWFromFile(name, "rb");
    if (!in) return out;
    char buf[65536];
    size_t n;
    while ((n = SDL_RWread(in, buf, 1, sizeof buf)) > 0) out.append(buf, n);
    SDL_RWclose(in);
    return out;
}

std::string readFile(const std::string& path) {
    std::string out;
    if (FILE* f = fopen(path.c_str(), "rb")) {
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
        fclose(f);
    }
    return out;
}

bool writeFile(const std::string& path, const std::string& data) {
    makeDirs(path.substr(0, path.find_last_of('/')));
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

void extractAssets(const std::string& dir) {
    std::string list = readAsset("medecomp-assets.txt");
    if (list.empty()) {
        fprintf(stderr, "medecomp: в APK нет ресурсов порта (medecomp-assets.txt)\n");
        return;
    }
    std::string stamp = list.substr(0, list.find('\n'));
    if (readFile(dir + "/.assets-stamp") == stamp) return;
    int count = 0;
    size_t pos = list.find('\n');
    while (pos != std::string::npos && pos + 1 < list.size()) {
        size_t end = list.find('\n', pos + 1);
        std::string name = list.substr(pos + 1, end == std::string::npos ? std::string::npos : end - pos - 1);
        pos = end;
        if (name.empty()) continue;
        std::string data = readAsset(name.c_str());
        if (writeFile(dir + "/" + name, data)) count++;
        else fprintf(stderr, "medecomp: ресурс %s не записать в %s\n", name.c_str(), dir.c_str());
    }
    writeFile(dir + "/.assets-stamp", stamp);
    fprintf(stderr, "medecomp: ресурсы порта (%d) — в %s\n", count, dir.c_str());
}

void fontConfig(const std::string& dir, const std::string& cache) {
    std::string conf = dir + "/fonts.conf";
    writeFile(conf, "<?xml version=\"1.0\"?>\n<!DOCTYPE fontconfig SYSTEM \"urn:fontconfig:fonts.dtd\">\n"
                    "<fontconfig>\n  <dir>/system/fonts</dir>\n  <dir>/product/fonts</dir>\n"
                    "  <cachedir>" + cache + "</cachedir>\n</fontconfig>\n");
    setenv("FONTCONFIG_FILE", conf.c_str(), 1);
}

void caBundle(const std::string& path) {
    std::string bundle;
    for (const char* dir : {"/apex/com.android.conscrypt/cacerts", "/system/etc/security/cacerts"}) {
        DIR* d = opendir(dir);
        if (!d) continue;
        while (dirent* e = readdir(d))
            if (e->d_name[0] != '.') bundle += readFile(std::string(dir) + "/" + e->d_name) + "\n";
        closedir(d);
        if (!bundle.empty()) break;
    }
    if (!bundle.empty() && writeFile(path, bundle)) setenv("SSL_CERT_FILE", path.c_str(), 0);
}

std::string findGame(const std::string& dir) {
    std::string found;
    if (DIR* d = opendir(dir.c_str())) {
        while (dirent* e = readdir(d)) {
            std::string name = e->d_name;
            size_t n = name.size();
            if (n > 4 && (name.compare(n - 4, 4, ".ipa") == 0 || name.compare(n - 4, 4, ".app") == 0)) {
                found = dir + "/" + name;
                break;
            }
        }
        closedir(d);
    }
    return found;
}

__attribute__((constructor(101))) void dataDirBeforeMain() {
    std::string package = readFile("/proc/self/cmdline");
    package = package.substr(0, package.find_first_of(std::string(":\0", 2)));
    if (package.empty()) return;
    const std::string env = readFile("/storage/emulated/0/Android/data/" + package + "/files/medecomp.env");
    for (size_t pos = 0; pos < env.size();) {
        size_t end = env.find('\n', pos);
        std::string line = env.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = end == std::string::npos ? env.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos || eq == 0) continue;
        setenv(line.substr(0, eq).c_str(), line.substr(eq + 1).c_str(), 1);
    }
    if (getenv("MEDECOMP_DATA")) return;
    const std::string files = "/data/data/" + package + "/files";
    setenv("MEDECOMP_DATA", files.c_str(), 0);
    setenv("HOME", files.c_str(), 0);
}

}

std::string androidPickFile(const std::string& dir, const char* mime) {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return std::string();
    jclass cls = env->GetObjectClass(activity);
    jmethodID pick = env->GetStaticMethodID(cls, "pickFile", "(Ljava/lang/String;Ljava/lang/String;)V");
    jmethodID picked = env->GetStaticMethodID(cls, "pickedFile", "()Ljava/lang/String;");
    std::string result;
    if (pick && picked) {
        makeDirs(dir);
        jstring jdir = env->NewStringUTF(dir.c_str());
        jstring jmime = env->NewStringUTF(mime);
        env->CallStaticVoidMethod(cls, pick, jdir, jmime);
        env->DeleteLocalRef(jdir);
        env->DeleteLocalRef(jmime);
        for (;;) {
            SDL_Delay(100);
            jstring s = static_cast<jstring>(env->CallStaticObjectMethod(cls, picked));
            const char* utf = s ? env->GetStringUTFChars(s, nullptr) : nullptr;
            std::string state = utf ? utf : "-";
            if (utf) env->ReleaseStringUTFChars(s, utf);
            if (s) env->DeleteLocalRef(s);
            if (state.empty()) continue;
            if (state != "-") result = state;
            break;
        }
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return result;
}

int androidDisplayRotation() {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return 0;
    jclass cls = env->GetObjectClass(activity);
    jmethodID rotation = env->GetStaticMethodID(cls, "displayRotation", "()I");
    int r = rotation ? env->CallStaticIntMethod(cls, rotation) : 0;
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return r & 3;
}

std::string androidSaveToDownloads(const std::string& path, const std::string& name, const char* mime) {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return std::string();
    std::string result;
    jclass cls = env->GetObjectClass(activity);
    jmethodID save = env->GetStaticMethodID(cls, "saveToDownloads", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (save) {
        jstring jpath = env->NewStringUTF(path.c_str()), jname = env->NewStringUTF(name.c_str()), jmime = env->NewStringUTF(mime);
        jstring s = static_cast<jstring>(env->CallStaticObjectMethod(cls, save, jpath, jname, jmime));
        if (s && !env->ExceptionCheck()) {
            const char* chars = env->GetStringUTFChars(s, nullptr);
            result = chars ? chars : "";
            env->ReleaseStringUTFChars(s, chars);
        }
        for (jstring j : {jpath, jname, jmime}) env->DeleteLocalRef(j);
        if (s) env->DeleteLocalRef(s);
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return result;
}

void androidRestart() {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return;
    jclass cls = env->GetObjectClass(activity);
    jmethodID restart = env->GetStaticMethodID(cls, "restart", "()V");
    if (restart) env->CallStaticVoidMethod(cls, restart);
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
}

void androidStartup() {
    const char* internal = SDL_AndroidGetInternalStoragePath();
    const std::string files = internal ? internal : "/data/local/tmp/medecomp";
    makeDirs(files);
    setenv("MEDECOMP_DATA", files.c_str(), 0);
    setenv("HOME", files.c_str(), 0);
    makeDirs(files + "/tmp");
    setenv("TMPDIR", (files + "/tmp").c_str(), 0);
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    logToLogcat(files + "/medecomp.log");
    const std::string app = files + "/app";
    extractAssets(app);
    if (chdir(app.c_str()) != 0) fprintf(stderr, "medecomp: нет папки ресурсов %s\n", app.c_str());
    fontConfig(app, files + "/fontconfig");
    caBundle(files + "/cacert.pem");
    std::string game = settings::get("game");
    struct stat st;
    if (!game.empty() && stat(game.c_str(), &st) != 0) game.clear();
    const char* external = SDL_AndroidGetExternalStoragePath();
    if (game.empty()) game = findGame(files + "/game");
    if (game.empty() && external) game = findGame(external);
    if (!game.empty() && !getenv("MEDECOMP_GAME")) setenv("MEDECOMP_GAME", game.c_str(), 1);
    fprintf(stderr, "medecomp: Android, папка игрока %s, копия игры %s\n", files.c_str(),
            game.empty() ? "не выбрана" : game.c_str());
}

}
}

#ifdef __LP64__
extern "C" void* valloc(size_t size) { return memalign(size_t(sysconf(_SC_PAGESIZE)), size); }
#endif

#endif
