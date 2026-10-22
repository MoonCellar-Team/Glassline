#include <stdio.h>

#include <string>

#include "port/cf.h"
#include "settings.h"

namespace settings = port::platform::settings;

extern "C" const char* medecomp_language(void) {
    static std::string language;
    language = settings::get("language", "auto");
    return language.empty() || language == "auto" ? nullptr : language.c_str();
}

extern "C" void medecomp_language_changed(const void* locale) {
    std::string now = port::cf::utf8(locale);
    std::string set = settings::get("language", "auto");
    if (now.empty() || set.empty() || set == "auto" || set == now) return;
    settings::set("language", now);
    if (!settings::save()) fprintf(stderr, "medecomp: не записать %s\n", settings::path().c_str());
    fprintf(stderr, "medecomp: язык игры: %s\n", now.c_str());
}
