#include "port/strings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <unistd.h>
#endif

#include <atomic>
#include <string>
#include <vector>

#include "game/LocaleManager.h"
#include "game/TextManager.h"
#include "runtime/midp/ReferenceCounted.h"
#include "runtime/midp/String.h"
#include "../platform/bindings.h"
#include "../platform/process.h"
#include "../platform/touch_device.h"

namespace {

namespace bindings = port::platform::bindings;

struct Row {
    int pool;
    int index;
    std::string lang;
    std::string text;
};

std::vector<Row> g_rows;
bool g_loaded = false;

struct Hint {
    int id;
    std::string lang;
    std::string text;
};

std::vector<Hint> g_hints;
unsigned char g_hintId[8192] = {};
std::atomic<int> g_input(0);
std::atomic<Uint32> g_fingerAt(0);
bool g_watch = false;

struct Shown {
    int id;
    std::string lang;
    std::string expanded;
    midp::String* str;
};

std::vector<Shown> g_shown;

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
const int kInputTexts[] = {0x0812, 0x0813, 0x085e, 0x096b};

bool inputText(int id, const std::string& text) {
    for (int t : kInputTexts)
        if (t == id) return true;
    for (const char* token : {"{up}", "{down}", "{left}", "{right}", "{tilt}", "{tap}", "{pause}"})
        if (text.find(token) != std::string::npos) return true;
    return false;
}
#endif

std::string touchRussian(int id, std::string text) {
    if (id == 0x0813) return "Коснитесь экрана";
    if (id == 0x085e) return "Проведите в обратную сторону, чтобы развернуться на ходу.";
    if (id == 0x096b) return "Коснитесь места в таблице, чтобы сразиться с призраком этого игрока.";
    const std::pair<const char*, const char*> replacements[] = {
        {"Нажмите {left} или {right}", "Проведите влево или вправо"},
        {"нажмите {left} или {right}", "проведите влево или вправо"},
        {"Нажмите {up}", "Проведите вверх"}, {"нажмите {up}", "проведите вверх"},
        {"Нажмите {down}", "Проведите вниз"}, {"нажмите {down}", "проведите вниз"},
        {"Нажмите {tap}", "Коснитесь экрана"}, {"нажмите {tap}", "коснитесь экрана"},
        {"Нажмите {pause}", "Проведите вниз двумя пальцами"},
        {"нажмите в обратную сторону", "проведите в обратную сторону"},
        {"Нажмите в сторону", "Проведите в сторону"}, {"нажмите в сторону", "проведите в сторону"},
        {"{left}", "свайп влево"}, {"{right}", "свайп вправо"},
        {"{up}", "свайп вверх"}, {"{down}", "свайп вниз"},
        {"{tap}", "касание экрана"}, {"{pause}", "свайп вниз двумя пальцами"},
        {"{tilt}", "наклоняйте устройство"},
    };
    for (const auto& replacement : replacements) {
        size_t pos = 0;
        while ((pos = text.find(replacement.first, pos)) != std::string::npos) {
            text.replace(pos, strlen(replacement.first), replacement.second);
            pos += strlen(replacement.second);
        }
    }
    return text;
}

void rememberHint(int id, const std::string& lang, const std::string& text) {
    if (id >= 0 && id < 8192)
        g_hintId[id] = 1;
    Hint hint;
    hint.id = id;
    hint.lang = lang;
    hint.text = text;
    g_hints.push_back(hint);
}

int onInputEvent(void*, SDL_Event* e) {
    if (e->type == SDL_KEYDOWN && !e->key.repeat && e->key.keysym.sym != SDLK_UNKNOWN)
        g_input.store(0, std::memory_order_relaxed);
    else if (e->type == SDL_CONTROLLERBUTTONDOWN)
        g_input.store(1, std::memory_order_relaxed);
    else if (e->type == SDL_CONTROLLERAXISMOTION) {
        int v = e->caxis.value;
        if (v > 8000 || v < -8000)
            g_input.store(1, std::memory_order_relaxed);
    }
    else if (e->type == SDL_FINGERDOWN || e->type == SDL_FINGERMOTION || e->type == SDL_FINGERUP) {
        g_fingerAt.store(SDL_GetTicks(), std::memory_order_relaxed);
        g_input.store(2, std::memory_order_relaxed);
    }
#ifdef __EMSCRIPTEN__
    else if ((e->type == SDL_MOUSEBUTTONDOWN && e->button.which != SDL_TOUCH_MOUSEID) ||
             (e->type == SDL_MOUSEMOTION && e->motion.which != SDL_TOUCH_MOUSEID)) {
        const Uint32 fingerAt = g_fingerAt.load(std::memory_order_relaxed);
        if (!fingerAt || SDL_GetTicks() - fingerAt >= 800) g_input.store(0, std::memory_order_relaxed);
    }
#endif
    return 0;
}

void ensureWatch() {
    if (g_watch)
        return;
    g_watch = true;
    if (port::platform::defaultTouchInput()) g_input.store(2, std::memory_order_relaxed);
    const char* env = getenv("MEDECOMP_HINT_PAD");
    if (env != NULL && env[0] == '1')
        g_input.store(1, std::memory_order_relaxed);
    SDL_AddEventWatch(onInputEvent, NULL);
}

const char* tiltPhrase(const std::string& lang) {
    if (lang == "fr")
        return "stick droit ←→";
    if (lang == "it")
        return "stick destro ←→";
    if (lang == "de")
        return "rechter Stick ←→";
    if (lang == "es_ES")
        return "stick derecho ←→";
    if (lang == "ja")
        return "右スティック←→";
    if (lang == "ko")
        return "오른쪽 스틱 ←→";
    if (lang == "zh_CN")
        return "右摇杆←→";
    if (lang == "ru")
        return "правый стик ←→";
    return "right stick ←→";
}

std::string dirLabel(bindings::Action action, bool pad, const char* arrow, const char* arrowName) {
    if (pad) {
        if (bindings::button(action) < 0)
            return arrow;
        return bindings::buttonLabel(action);
    }
    const char* name = bindings::keyLabel(action);
    if (strcmp(name, arrowName) == 0)
        return arrow;
    std::string label = name;
    label += "/";
    label += arrow;
    return label;
}

std::string token(const std::string& name, bool pad, const std::string& lang) {
    if (name == "up")
        return dirLabel(bindings::ActUp, pad, "↑", "Up");
    if (name == "down")
        return dirLabel(bindings::ActDown, pad, "↓", "Down");
    if (name == "left")
        return dirLabel(bindings::ActLeft, pad, "←", "Left");
    if (name == "right")
        return dirLabel(bindings::ActRight, pad, "→", "Right");
    if (name == "tap") {
        if (pad)
            return bindings::button(bindings::ActTap) < 0 ? "A" : bindings::buttonLabel(bindings::ActTap);
        return bindings::keyLabel(bindings::ActTap);
    }
    if (name == "pause") {
        if (pad)
            return bindings::button(bindings::ActPause) < 0 ? "Start" : bindings::buttonLabel(bindings::ActPause);
        return bindings::keyLabel(bindings::ActPause);
    }
    if (name == "tilt") {
        if (pad) {
            int left = bindings::button(bindings::ActTiltLeft);
            int right = bindings::button(bindings::ActTiltRight);
            if (left < 0 && right < 0)
                return tiltPhrase(lang);
            std::string label = left < 0 ? "—" : bindings::buttonLabel(bindings::ActTiltLeft);
            label += "/";
            label += right < 0 ? "—" : bindings::buttonLabel(bindings::ActTiltRight);
            return label;
        }
        std::string left = bindings::keyLabel(bindings::ActTiltLeft);
        std::string right = bindings::keyLabel(bindings::ActTiltRight);
        return left + "/" + right;
    }
    return "{" + name + "}";
}

std::string expand(const std::string& templ, bool pad, const std::string& lang) {
    std::string out;
    out.reserve(templ.size() + 16);
    for (size_t i = 0; i < templ.size();) {
        if (templ[i] != '{') {
            out.push_back(templ[i++]);
            continue;
        }
        size_t end = templ.find('}', i + 1);
        if (end == std::string::npos) {
            out.push_back(templ[i++]);
            continue;
        }
        out += token(templ.substr(i + 1, end - (i + 1)), pad, lang);
        i = end + 1;
    }
    return out;
}

midp::String* internHint(int id, const std::string& lang, const std::string& expanded) {
    for (size_t i = 0; i < g_shown.size(); i++) {
        if (g_shown[i].id == id && g_shown[i].lang == lang && g_shown[i].expanded == expanded)
            return g_shown[i].str;
    }
    int n = (int)expanded.size();
    midp::array<signed char> bytes(n);
    for (int c = 0; c < n; c++)
        bytes[c] = (signed char)(unsigned char)expanded[(size_t)c];
    midp::String* s = new midp::String(bytes, "UTF-8");
    midp::INCREF(s);
    Shown shown;
    shown.id = id;
    shown.lang = lang;
    shown.expanded = expanded;
    shown.str = s;
    g_shown.push_back(shown);
    return s;
}

bool readable(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f == NULL)
        return false;
    fclose(f);
    return true;
}

std::string tablePath() {
    const char* env = getenv("MEDECOMP_STRINGS");
    if (env != NULL && env[0] != '\0')
        return env;
    std::string dirs[7];
    int count = 0;
    dirs[count++] = ".";
#if defined(__linux__) || defined(_WIN32) || defined(__APPLE__)
    std::string self = port::platform::process::selfPath();
    if (!self.empty()) {
        std::string dir(self);
        std::string::size_type slash = dir.find_last_of('/');
        if (slash != std::string::npos)
            dir.erase(slash);
#ifdef __APPLE__
        dirs[count++] = dir + "/../Resources";
#endif
        for (int i = 0; i < 5 && count < 7 && !dir.empty(); i++) {
            dirs[count++] = dir;
            slash = dir.find_last_of('/');
            if (slash == std::string::npos || slash == 0)
                break;
            dir.erase(slash);
        }
    }
#endif
    for (int i = 0; i < count; i++) {
        std::string path = dirs[i] + "/config/string_overrides.tsv";
        if (readable(path))
            return path;
    }
    return "";
}

bool parseInt(const std::string& s, int* out) {
    if (s.empty())
        return false;
    const char* p = s.c_str();
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
        base = 16;
    char* end = NULL;
    long value = strtol(p, &end, base);
    if (end == p || *end != '\0')
        return false;
    *out = (int)value;
    return true;
}

struct Translation {
    const char* file;
    const char* lang;
};
const Translation kTranslations[] = {{"strings_ru.tsv", "ru"}};

std::string translationPath(const std::string& table, const char* file) {
    std::string::size_type slash = table.find_last_of("/\\");
    std::string path = (slash == std::string::npos ? std::string() : table.substr(0, slash + 1)) + file;
    return readable(path) ? path : "";
}

void readTable(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f == NULL) {
        fprintf(stderr, "%s: не открыть — строки игры без замен\n", path.c_str());
        return;
    }
    const int bits = TextManager::STRING_ID_BITS;
    const int indexMask = (1 << bits) - 1;
    char line[65536];
    int number = 0;
    while (fgets(line, (int)sizeof(line), f) != NULL) {
        number++;
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0 || line[0] == '#')
            continue;
        char* tab1 = strchr(line, '\t');
        char* tab2 = tab1 != NULL ? strchr(tab1 + 1, '\t') : NULL;
        char* tab3 = tab2 != NULL ? strchr(tab2 + 1, '\t') : NULL;
        if (tab3 == NULL) {
            fprintf(stderr, "%s:%d: нужны пул, номер, язык и текст через таб\n", path.c_str(), number);
            continue;
        }
        *tab1 = '\0';
        *tab2 = '\0';
        *tab3 = '\0';
        int pool = 0;
        int id = 0;
        if (!parseInt(line, &pool) || !parseInt(tab1 + 1, &id)) {
            fprintf(stderr, "%s:%d: пул или номер не число\n", path.c_str(), number);
            continue;
        }
        if ((id >> bits) != pool) {
            fprintf(stderr, "%s:%d: пул %d не совпадает с номером\n", path.c_str(), number, pool);
            continue;
        }
        Row row;
        row.pool = pool;
        row.index = id & indexMask;
        row.lang = tab2 + 1;
        row.text = tab3 + 1;
        g_rows.push_back(row);
        if (row.text.find('{') != std::string::npos
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
            || inputText(id, row.text)
#endif
        ) rememberHint(id, row.lang, row.text);
    }
    fclose(f);
}

void loadTable() {
    g_loaded = true;
    std::string path = tablePath();
    if (path.empty()) {
        fprintf(stderr, "config/string_overrides.tsv не найден — строки игры без замен\n");
        return;
    }
    readTable(path);
    for (const Translation& t : kTranslations) {
        std::string translation = translationPath(path, t.file);
        if (!translation.empty())
            readTable(translation);
    }
}

}

namespace port {

std::vector<std::string> tableLocales() {
    std::vector<std::string> out;
    std::string path = tablePath();
    if (path.empty())
        return out;
    for (const Translation& t : kTranslations)
        if (!translationPath(path, t.file).empty())
            out.push_back(t.lang);
    return out;
}

void applyStringOverrides(int pool, midp::array<midp::ReferenceCountedPointer<midp::String>>& strings,
                          midp::String* locale) {
    if (!g_loaded)
        loadTable();
    if (g_rows.empty())
        return;
    if (locale == NULL)
        locale = LocaleManager::getInstance()->getLocale();
    if (locale == NULL)
        return;
    int needed = strings.m_length;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    const int poolSize = strings.m_length;
#endif
    for (size_t i = 0; i < g_rows.size(); i++)
        if (g_rows[i].pool == pool && g_rows[i].index >= needed && g_rows[i].index < 2048)
            needed = g_rows[i].index + 1;
    if (needed > strings.m_length) {
        int old = strings.m_length;
        midp::array<midp::ReferenceCountedPointer<midp::String>> bigger(needed);
        for (int k = 0; k < old; k++)
            bigger[k] = strings[k];
        for (size_t i = 0; i < g_rows.size(); i++) {
            const Row& row = g_rows[i];
            if (row.pool != pool || row.index < old || row.lang != "en")
                continue;
            int n = (int)row.text.size();
            midp::array<signed char> bytes(n);
            for (int c = 0; c < n; c++)
                bytes[c] = (signed char)(unsigned char)row.text[(size_t)c];
            bigger[row.index] = new midp::String(bytes, "UTF-8");
        }
        strings = bigger;
    }
    for (size_t i = 0; i < g_rows.size(); i++) {
        const Row& row = g_rows[i];
        if (row.pool != pool || !locale->equals(row.lang.c_str()))
            continue;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
        if (row.index < poolSize && inputText((row.pool << 11) | row.index, row.text))
            continue;
#endif
        if (row.index < 0 || row.index >= strings.m_length) {
            fprintf(stderr, "замена %d:%s — в пуле %d нет строки %d\n", row.pool, row.lang.c_str(), pool,
                    row.index);
            continue;
        }
        int n = (int)row.text.size();
        midp::array<signed char> bytes(n);
        for (int c = 0; c < n; c++)
            bytes[c] = (signed char)(unsigned char)row.text[(size_t)c];
        strings[row.index] = new midp::String(bytes, "UTF-8");
    }
}

midp::String* withInputHints(int textId, midp::String* raw, midp::String* locale) {
    if (raw == NULL || textId < 0 || textId >= 8192 || g_hintId[textId] == 0)
        return raw;
    if (!g_loaded)
        loadTable();
    if (locale == NULL)
        locale = LocaleManager::getInstance()->getLocale();
    if (locale == NULL)
        return raw;
    const Hint* hint = NULL;
    for (size_t i = 0; i < g_hints.size(); i++) {
        if (g_hints[i].id == textId && locale->equals(g_hints[i].lang.c_str())) {
            hint = &g_hints[i];
            break;
        }
    }
    if (hint == NULL)
        return raw;
    ensureWatch();
    const int device = g_input.load(std::memory_order_relaxed);
    bool pad = device == 1;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    const bool touch =
#ifdef __ANDROID__
        !pad;
#else
        device == 2;
#endif
    if (touch && inputText(textId, hint->text)) {
        if (hint->lang == "ru") return internHint(textId, hint->lang, touchRussian(textId, hint->text));
        return raw;
    }
#endif
    std::string text = expand(hint->text, pad, hint->lang);
    static int logged = -1;
    int mode = pad ? 1 : 0;
    if (logged != mode) {
        logged = mode;
        std::string up = token("up", pad, hint->lang);
        std::string tilt = token("tilt", pad, hint->lang);
        fprintf(stderr, "medecomp: подсказки ввода: %s, вверх %s, наклон %s\n", pad ? "геймпад" : "клавиатура",
                up.c_str(), tilt.c_str());
    }
    return internHint(textId, hint->lang, text);
}

}
