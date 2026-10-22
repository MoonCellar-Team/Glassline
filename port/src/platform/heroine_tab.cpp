#include "heroine_tab.h"

#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../ea/cloud.h"
#include "../gfx/png.h"
#include "file_dialog.h"
#include "gltf.h"
#include "autorig.h"
#include "heroine_model.h"
#include "imgui.h"
#include "mcui.h"
#include "port/files.h"
#include "port/platform.h"
#include "process.h"
#include "skins.h"

namespace skins = port::platform::skins;
namespace process = port::platform::process;

namespace port {
namespace platform {
namespace heroine {

namespace {

bool s_russian;
const char* L(const char* ru, const char* en) { return s_russian ? ru : en; }

std::string s_game;
long s_child = 0;
std::string s_export;
bool s_offscreenFailed = false;
std::string s_pendingSelect;
std::string s_pendingFolder;
std::string s_error;
std::string s_note;
std::vector<skins::Choice> s_choices;
std::map<std::string, SDL_Texture*> s_textures;
std::map<std::string, ImVec2> s_sizes;
bool s_reload = true;
bool s_tabShown = false;
#ifdef __ANDROID__
bool s_stale = false;
std::vector<std::string> s_previewEnv;
bool s_previewPending = false;

std::string externalDir() {
    const char* path = SDL_AndroidGetExternalStoragePath();
    return path ? path : std::string();
}
#endif

std::string cacheDir() { return port::files::dataDir() + "/cache/skins"; }

void makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++)
        if (i == path.size() || path[i] == '/') {
#ifdef _WIN32
            mkdir(path.substr(0, i).c_str());
#else
            mkdir(path.substr(0, i).c_str(), 0755);
#endif
        }
}

long mtime(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 ? long(st.st_mtime) : 0;
}

std::string readFile(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

void start(const std::string& exportSpec) {
    if (s_child || s_game.empty()) return;
#ifdef __ANDROID__
    makeDirs(cacheDir());
    remove((cacheDir() + "/done").c_str());
    s_previewEnv = {"MEDECOMP_SKIN_PREVIEWS=" + cacheDir(), "MEDECOMP_SKIN_TEMPLATE=" + cacheDir() + "/faith-template.glb",
                    "ALSOFT_DRIVERS=null"};
    if (!exportSpec.empty()) s_previewEnv.push_back("MEDECOMP_SKIN_EXPORT=" + exportSpec);
    s_previewPending = true;
    return;
#endif
    std::string self = process::selfPath();
    if (self.empty()) {
        s_error = L("не найти свой исполняемый файл", "cannot find own executable");
        return;
    }
    makeDirs(cacheDir());
    remove((cacheDir() + "/done").c_str());
    std::vector<std::string> env = {"MEDECOMP_SKIN_PREVIEWS=" + cacheDir(), "MEDECOMP_WINDOW_SIZE=1024x768",
                                    "MEDECOMP_SKIN_TEMPLATE=" + cacheDir() + "/faith-template.glb",
                                    "ALSOFT_DRIVERS=null"};
    if (!s_offscreenFailed) env.push_back("SDL_VIDEODRIVER=offscreen");
    if (!exportSpec.empty()) env.push_back("MEDECOMP_SKIN_EXPORT=" + exportSpec);
    s_export = exportSpec;
    s_child = process::spawn({self, s_game}, env);
    if (!s_child) s_error = L("не запустить игру для превью", "cannot start the game for previews");
}

void loadTextures(SDL_Renderer* renderer) {
    for (auto& t : s_textures) SDL_DestroyTexture(t.second);
    s_textures.clear();
    s_sizes.clear();
    for (const skins::Choice& c : s_choices) {
        std::string bytes = readFile(cacheDir() + "/" + skins::previewFile(c));
        port::gfx::Image image;
        if (bytes.empty() ||
            !port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), image, false))
            continue;
        SDL_Texture* tex =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
        if (!tex) continue;
        SDL_UpdateTexture(tex, nullptr, image.rgba.data(), image.width * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
        s_textures[c.key] = tex;
        s_sizes[c.key] = ImVec2(float(image.width), float(image.height));
    }
}

std::string baseOfSelected() {
    std::string base = skins::selected();
    for (const skins::Choice& c : s_choices)
        if (c.key == base && c.custom) {
            std::string model = readFile(skins::folder() + "/" + c.key + "/model");
            while (!model.empty() && (model.back() == '\n' || model.back() == '\r' || model.back() == ' '))
                model.pop_back();
            base = model;
        }
    if (base.empty() || base.compare(0, 5, "game:") != 0) base = "game:faith-day";
    return base;
}

std::string newSkin(const std::string& base, const std::string& name, std::string* id) {
    int n = 1;
    for (;; n++) {
        *id = "skin-" + std::to_string(n);
        struct stat st;
        if (stat((skins::folder() + "/" + *id).c_str(), &st) != 0) break;
    }
    std::string dir = skins::folder() + "/" + *id;
    makeDirs(dir);
    std::string label = name;
    if (label.empty()) {
        for (const skins::Choice& c : s_choices)
            if (c.key == base) label = s_russian ? c.ru : c.en;
        label += std::string(" — ") + L("свой", "custom") + " " + std::to_string(n);
    }
    std::ofstream((dir + "/name").c_str()) << label << "\n";
    std::ofstream((dir + "/model").c_str()) << base << "\n";
    return dir;
}

bool skinLimitReached() {
    namespace cloud = port::online::cloud;
    if (!cloud::skinLimitActive() || cloud::customSkins() < cloud::kSkinLimit) return false;
    s_error = std::string(L("своих скинов уже ", "there are already ")) + std::to_string(cloud::kSkinLimit) +
              L(" — больше нельзя, пока скины синхронизируются с аккаунтом MoonCellar (модели и текстуры тяжёлые). "
                "Удалите ненужный скин («Удалить скин») или выключите «Синхронизировать свои скины» во вкладке «Аккаунт».",
                " custom skins — no more while skins sync with the MoonCellar account (models and textures are heavy). "
                "Delete one (Delete skin) or turn off Sync custom skins in the Account tab.");
    fprintf(stderr, "medecomp: свой скин не создан — лимит %d при синхронизации скинов\n", cloud::kSkinLimit);
    return true;
}

void createSkin() {
    if (skinLimitReached()) return;
    std::string base = baseOfSelected(), id;
    std::string dir = newSkin(base, std::string(), &id);
    skins::rescan();
    s_choices = skins::list();
    s_pendingSelect = id;
    s_pendingFolder = dir;
#ifdef __ANDROID__
    skins::select(id);
#endif
    start(base + "|" + dir + "/texture.png");
}

void importTexture(const std::string& path) {
    std::string bytes = readFile(path);
    port::gfx::Image image;
    if (bytes.empty() ||
        !port::gfx::decodePNG(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), image, false)) {
        s_error = std::string(L("не PNG или не прочитать: ", "not a PNG or unreadable: ")) + path;
        return;
    }
    std::string chosen = skins::selected(), dir;
    for (const skins::Choice& c : s_choices)
        if (c.key == chosen && c.custom) dir = skins::folder() + "/" + c.key;
    if (dir.empty()) {
        if (skinLimitReached()) return;
        std::string stem = path.substr(path.find_last_of("/\\") + 1);
        stem = stem.substr(0, stem.find_last_of('.'));
        dir = newSkin(baseOfSelected(), stem, &chosen);
    }
    std::ofstream out((dir + "/texture.png").c_str(), std::ios::binary);
    out.write(bytes.data(), std::streamsize(bytes.size()));
    if (!out) {
        s_error = std::string(L("не записать ", "cannot write ")) + dir + "/texture.png";
        return;
    }
    out.close();
    s_error.clear();
    s_note = image.width == 512 && image.height == 512
                 ? std::string()
                 : std::string(L("Картинка ", "The image is ")) + std::to_string(image.width) + "×" +
                       std::to_string(image.height) +
                       L(": у модели — 512×512. Ляжет, если раскладка та же, что у оригинала (растягивается).",
                         ": the model's is 512×512. It fits if the layout is the original's (it gets scaled).");
    skins::rescan();
    s_choices = skins::list();
    skins::select(chosen);
    s_reload = true;
}

}

void update(SDL_Renderer* renderer, const std::string& game) {
    if (game != s_game) {
        s_game = game;
        s_reload = true;
    }
    if (s_child) {
        int code = process::poll(s_child);
        if (code < 0) return;
        s_child = 0;
        if (code != 0 && !s_offscreenFailed) {
            s_offscreenFailed = true;
            start(s_export);
            return;
        }
        if (code == 0) {
            if (!s_pendingSelect.empty()) {
                skins::rescan();
                s_choices = skins::list();
                skins::select(s_pendingSelect);
                port::platform::openURL(("file://" + s_pendingFolder).c_str());
                s_pendingSelect.clear();
                s_pendingFolder.clear();
            }
        } else {
            s_error = L("превью не вышли — журнал игры в папке игрока", "previews failed — see the game log");
        }
        loadTextures(renderer);
        return;
    }
    if (!s_reload) return;
    s_reload = false;
    s_choices = skins::list();
    if (s_game.empty()) return;
    static bool imported = false;
    if (const char* file = getenv("MEDECOMP_HEROINE_IMPORT"); file && *file && !imported && s_tabShown) {
        imported = true;
        std::string f = file;
        picked(f.size() > 4 && f.compare(f.size() - 4, 4, ".png") == 0, f);
        if (s_reload) return update(renderer, game);
    }
    bool fresh = readFile(cacheDir() + "/stamp") == skins::previewStamp(port::files::gameSource()) &&
                 mtime(cacheDir() + "/done") != 0;
    loadTextures(renderer);
#ifdef __ANDROID__
    s_stale = !fresh;
#else
    if (!fresh) start(std::string());
#endif
}

void exportTemplate();
void deleteSkin(const std::string& key);

void tab(float scale, bool russian) {
    s_russian = russian;
    if (!s_tabShown) s_tabShown = true, s_reload = true;
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", L("Модель героини в забеге. Соперники, полиция и призрак — как раньше. Свой скин — "
                               "модель игры со своей текстурой (texture.png: «Свой скин из текстуры» или «Импорт "
                               "текстуры…») или своя модель glTF или VRM («Импорт модели…»): со своим скелетом "
                               "(Mixamo, VRM, Unreal, Blender…) или без него она привязывается к скелету Фейт сама; "
                               "точнее всего — привязать в Blender к «Шаблону для Blender».",
                               "The heroine's model in runs. Rivals, police and the ghost stay as they are. A "
                               "custom skin is a game model with your texture (texture.png: Custom skin from "
                               "texture or Import texture…) or your own glTF or VRM model (Import model…): with its "
                               "own skeleton (Mixamo, VRM, Unreal, Blender…) or without one it is bound to Faith's "
                               "skeleton automatically; the most precise way is to bind it in Blender to the "
                               "Template for Blender."));
    ImGui::PopStyleColor();
    if (s_game.empty())
        ImGui::TextWrapped("%s", L("Сначала укажите копию игры на вкладке «Игра».", "Choose the game copy on the Game tab first."));
    else if (s_child)
        ImGui::TextWrapped("%s", L("Готовлю превью…", "Rendering previews…"));
    auto colored = [](ImVec4 color, const std::string& text) {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::PopStyleColor();
    };
    if (port::online::cloud::skinLimitActive())
        ImGui::TextDisabled("%s %d / %d", L("Своих скинов (синхронизируются с аккаунтом):", "Custom skins (synced with the account):"),
                            port::online::cloud::customSkins(), port::online::cloud::kSkinLimit);
    if (!s_error.empty()) colored(mcui::Color(mcui::tokens::kColorNegative), s_error);
    if (!s_note.empty()) colored(mcui::Color(mcui::tokens::kColorAttention), s_note);
#ifdef __ANDROID__
    if (s_stale && !s_game.empty()) {
        colored(mcui::Color(mcui::tokens::kColorAttention),
                L("Превью скинов (и шаблон для Blender) рисует сама игра: приложение закроется и откроется снова, "
                  "это около полуминуты.",
                  "The game itself renders the skin previews (and the Blender template): the app closes and opens "
                  "again, about half a minute."));
        if (mcui::Button(L("Подготовить превью", "Prepare previews"), mcui::ButtonKind::Accent)) start(std::string());
    }
    {
        std::string current = skins::selected();
        std::string shown = current.empty() ? std::string() : readFile(cacheDir() + "/exported-" + current);
        if (!shown.empty())
            ImGui::TextWrapped("%s%s%s", L("Основа текстуры этого скина — ", "This skin's texture base is "), shown.c_str(),
                               L(": правьте её в любом редакторе картинок и ставьте «Импорт текстуры…».",
                                 ": edit it in any image editor and install it with Import texture…."));
    }
#endif

    ImGui::BeginDisabled(s_game.empty() || s_child != 0);
    if (ImGui::Button(L("Свой скин из текстуры", "Custom skin from texture"))) createSkin();
    ImGui::SameLine();
    ImGui::BeginDisabled(port::platform::dialog::busy());
    if (ImGui::Button(L("Импорт текстуры…", "Import texture…")))
        port::platform::dialog::pick(port::platform::dialog::Png, std::string(),
                                     L("Текстура героини (PNG)", "Heroine texture (PNG)"));
    ImGui::SameLine();
    if (ImGui::Button(L("Импорт модели…", "Import model…")))
        port::platform::dialog::pick(port::platform::dialog::Gltf, std::string(),
                                     L("Своя модель героини (glTF 2.0, VRM)", "Custom heroine model (glTF 2.0, VRM)"));
    ImGui::EndDisabled();
    if (ImGui::Button(L("Шаблон для Blender", "Template for Blender"))) exportTemplate();
    ImGui::SameLine();
    if (ImGui::Button(L("Обновить", "Refresh"))) {
        skins::rescan();
        remove((cacheDir() + "/stamp").c_str());
        s_error.clear();
        s_reload = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button(L("Папка скинов", "Skins folder"))) {
        makeDirs(skins::folder());
#ifdef __ANDROID__
        s_note = L("Свои скины лежат в папке приложения — их ставят «Импорт текстуры…» и «Импорт модели…» из любой папки "
                   "телефона. Шаблон для Blender и основы текстур игра кладёт в «Загрузки», папку MoonCellar.",
                   "Custom skins live in the app's folder — install them with Import texture… and Import model… from any "
                   "folder on the phone. The game puts the Blender template and texture bases into Downloads, the "
                   "MoonCellar folder.");
#else
        port::platform::openURL(("file://" + skins::folder()).c_str());
#endif
    }
    std::string current = skins::selected();
    bool custom = false;
    for (const skins::Choice& c : s_choices) custom |= c.key == current && c.custom;
    ImGui::SameLine();
    ImGui::BeginDisabled(!custom || s_child != 0);
    if (ImGui::Button(L("Удалить скин", "Delete skin"))) ImGui::OpenPopup("##delete-skin");
    ImGui::EndDisabled();
    if (ImGui::BeginPopup("##delete-skin")) {
        std::string label;
        for (const skins::Choice& c : s_choices)
            if (c.key == current) label = s_russian ? c.ru : c.en;
        ImGui::Text("%s «%s»?", L("Удалить скин", "Delete skin"), label.c_str());
        if (ImGui::Button(L("Удалить", "Delete"))) {
            deleteSkin(current);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(L("Отмена", "Cancel"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Spacing();

    std::string chosen = skins::selected();
    const float cardW = 150.0f * scale, imageH = 200.0f * scale, pad = 8.0f * scale;
    const float cardH = imageH + ImGui::GetTextLineHeightWithSpacing() * 2.0f;
    int perRow = std::max(1, int((ImGui::GetContentRegionAvail().x + pad) / (cardW + pad)));
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < s_choices.size(); i++) {
        const skins::Choice& c = s_choices[i];
        ImVec2 p(origin.x + float(i % size_t(perRow)) * (cardW + pad), origin.y + float(i / size_t(perRow)) * (cardH + pad));
        ImGui::SetCursorScreenPos(p);
        ImGui::PushID(int(i));
        bool selected = c.key == chosen;
        if (ImGui::InvisibleButton("##card", ImVec2(cardW, cardH), ImGuiButtonFlags_EnableNav)) skins::select(c.key);
        bool hovered = ImGui::IsItemHovered();
        ImU32 back = ImGui::GetColorU32(selected ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_FrameBg);
        ImVec2 q(p.x + cardW, p.y + cardH);
        draw->AddRectFilled(p, q, back, 6.0f * scale);
        if (selected)
            draw->AddRect(p, q, ImGui::GetColorU32(mcui::Color(mcui::tokens::kColorAccent)), 6.0f * scale, 0,
                          2.0f * scale);
        auto tex = s_textures.find(c.key);
        if (tex != s_textures.end()) {
            ImVec2 size = s_sizes[c.key];
            float k = std::min((cardW - 2 * pad) / size.x, (imageH - 2 * pad) / size.y);
            ImVec2 a(p.x + (cardW - size.x * k) / 2, p.y + pad + (imageH - 2 * pad - size.y * k));
            draw->AddImage(ImTextureRef((ImTextureID)(intptr_t)tex->second), a, ImVec2(a.x + size.x * k, a.y + size.y * k));
        }
        const std::string& name = s_russian ? c.ru : c.en;
        float wrap = cardW - 2 * pad;
        ImVec2 ts = ImGui::CalcTextSize(name.c_str(), nullptr, false, wrap);
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p.x + (cardW - ts.x) / 2, p.y + imageH),
                      ImGui::GetColorU32(ImGuiCol_Text), name.c_str(), nullptr, wrap);
        ImGui::PopID();
    }
    size_t rows = (s_choices.size() + size_t(perRow) - 1) / size_t(perRow);
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(ImVec2(float(perRow) * (cardW + pad), float(rows) * (cardH + pad)));
}

std::string boneWords(const std::string& bone) {
    static const struct {
        const char *name, *ru, *en;
    } kWords[] = {
        {"pelvis", "таз", "pelvis"}, {"head", "голова", "head"},
        {"upperarm_l", "левое плечо", "left upper arm"}, {"upperarm_r", "правое плечо", "right upper arm"},
        {"forearm_l", "левое предплечье", "left forearm"}, {"forearm_r", "правое предплечье", "right forearm"},
        {"thigh_l", "левое бедро", "left thigh"}, {"thigh_r", "правое бедро", "right thigh"},
        {"calf_l", "левая голень", "left calf"}, {"calf_r", "правая голень", "right calf"},
        {"?", "скелет не понят", "the skeleton is not understood"},
    };
    for (const auto& w : kWords)
        if (bone == w.name) return L(w.ru, w.en);
    return bone;
}

bool onTemplate(const port::platform::gltf::Scene& scene, const port::platform::gltf::Model& faith,
                port::platform::gltf::Model* model) {
    std::vector<int> map;
    std::string error;
    if (!port::platform::gltf::toModel(scene, model, &error) || !checkModel(*model, &map, &error)) return false;
    for (size_t b = 0; b < model->bones.size(); b++) {
        const float* m = &model->inverseBind[b * 16];
        const float* f = &faith.inverseBind[size_t(map[b]) * 16];
        for (int i = 0; i < 16; i++)
            if (fabsf(m[i] - f[i]) > 0.02f) return false;
    }
    return true;
}

void importModel(const std::string& path) {
    if (skinLimitReached()) return;
    namespace gltf = port::platform::gltf;
    gltf::setRussian(s_russian);
    gltf::Scene scene;
    std::string error;
    if (!gltf::readScene(path, &scene, &error)) {
        s_error = std::string(L("модель не подходит: ", "the model does not fit: ")) + error;
        fprintf(stderr, "medecomp: импорт модели %s — %s\n", path.c_str(), s_error.c_str());
        return;
    }
    gltf::Model faith;
    if (!gltf::read(cacheDir() + "/faith-template.glb", &faith, &error)) {
        s_error = L("шаблона Фейт ещё нет — дождитесь превью", "no Faith template yet — wait for the previews");
        return;
    }
    gltf::Model model;
    autorig::Report report;
    bool asIs = onTemplate(scene, faith, &model);
    if (!asIs) {
        model = gltf::Model();
        if (!autorig::fit(scene, faith, &model, &report, &error)) {
            s_error = std::string(L("модель не подходит: ", "the model does not fit: ")) + error;
            fprintf(stderr, "medecomp: импорт модели %s — %s\n", path.c_str(), s_error.c_str());
            return;
        }
    }
    std::string stem = path.substr(path.find_last_of("/\\") + 1), id;
    stem = stem.substr(0, stem.find_last_of('.'));
    std::string dir = newSkin("game:faith-day", stem, &id);
    if (!gltf::write(dir + "/model.glb", model, &error)) {
        s_error = error;
        return;
    }
    s_error.clear();
    s_note.clear();
    if (asIs) {
        if (!model.hasImage())
            s_note = L("В модели нет текстуры: положите texture.png в папку скина или «Импорт текстуры…».",
                       "The model has no texture: put texture.png into the skin folder or use Import texture….");
    } else {
        if (report.bySkeleton) {
            s_note = std::string(L("Автопривязка: скелет", "Auto-rig: skeleton")) + (report.rig.empty() ? "" : " " + report.rig) +
                     L(", костей сопоставлено: ", ", bones matched: ") + std::to_string(report.mapped) +
                     L(", слито в ближайшие (пальцы, волосы, одежда): ", ", merged into the nearest (fingers, hair, clothes): ") +
                     std::to_string(report.merged) + ".";
            if (report.transferred)
                s_note += std::string(" ") + L("Вершин без костей Фейт (веса — с тела Фейт): ", "Vertices without Faith's bones (weights from Faith's body): ") +
                          std::to_string(report.transferred) + ".";
        } else {
            std::string missing;
            for (const std::string& b : report.missing) missing += (missing.empty() ? "" : ", ") + boneWords(b);
            s_note = std::string(L("Автопривязка: ", "Auto-rig: ")) +
                     (scene.skins.empty() ? std::string(L("скелета нет", "no skeleton"))
                                          : std::string(L("в скелете нет: ", "the skeleton lacks: ")) + missing) +
                     L(" — суставы найдены по форме тела, веса взяты с тела Фейт.",
                       " — joints found by the body shape, weights taken from Faith's body.");
        }
        if (report.turned) s_note += std::string(" ") + L("Модель повёрнута, как стоит Фейт.", "The model was turned to stand like Faith.");
        if (report.downscaled)
            s_note += std::string(" ") + L("Картинки уменьшены: скин уходит сопернику по сети (до 8 МБ).",
                                           "Images were downscaled: the skin is sent to the online rival (up to 8 MB).");
        s_note += std::string(" ") + L("Модель встала в позу Фейт и двигается её анимациями; длинная одежда (юбка, плащ, "
                                       "длинные волосы) может тянуться за ногами. Не понравилось — «Удалить скин».",
                                       "The model now has Faith's pose and moves with her animations; long clothes (a "
                                       "skirt, a cloak, long hair) may stretch with the legs. Not happy — Delete skin.");
    }
    fprintf(stderr, "medecomp: импорт модели %s — скин %s, %zu вершин, частей %zu. %s\n", path.c_str(), id.c_str(),
            model.vertexCount(), model.parts.size(), s_note.c_str());
    skins::rescan();
    s_choices = skins::list();
    skins::select(id);
    s_reload = true;
}

void deleteSkin(const std::string& key) {
    std::string dir = skins::folder() + "/" + key;
    for (const std::string& name : port::files::list(dir)) remove((dir + "/" + name).c_str());
    if (rmdir(dir.c_str()) != 0) {
        s_error = std::string(L("не удалить ", "cannot delete ")) + dir;
        return;
    }
    s_error.clear();
    s_note.clear();
    skins::select(std::string());
    skins::rescan();
    s_choices = skins::list();
    remove((cacheDir() + "/stamp").c_str());
    s_reload = true;
}

void exportTemplate() {
    std::string bytes = readFile(cacheDir() + "/faith-template.glb");
    if (bytes.empty()) {
        s_error = L("шаблона ещё нет — дождитесь превью", "no template yet — wait for the previews");
        return;
    }
#ifdef __ANDROID__
    std::string shown = androidSaveToDownloads(cacheDir() + "/faith-template.glb", "faith-template.glb", "model/gltf-binary");
    if (shown.empty()) {
        shown = externalDir() + "/faith-template.glb";
        std::ofstream(shown.c_str(), std::ios::binary).write(bytes.data(), std::streamsize(bytes.size()));
    }
    s_note = std::string(L("Шаблон — ", "The template is ")) + shown +
             L(": на компьютере (по USB) привяжите к этому скелету свою модель (кости и позу не менять), выгрузите в "
               "glTF 2.0 (.glb) с текстурой, скопируйте на телефон и — «Импорт модели…».",
               ": on a computer (over USB) bind your model to this skeleton (keep the bones and the pose), export glTF "
               "2.0 (.glb) with a texture, copy it to the phone and use Import model….");
    return;
#endif
    makeDirs(skins::folder());
    std::ofstream((skins::folder() + "/faith-template.glb").c_str(), std::ios::binary).write(bytes.data(), std::streamsize(bytes.size()));
    s_note = L("faith-template.glb — в папке скинов: привяжите свою модель к этому скелету (кости и позу не менять) и "
               "выгрузите в glTF 2.0 (.glb) с текстурой; затем «Импорт модели…».",
               "faith-template.glb is in the skins folder: bind your model to this skeleton (keep the bones and the "
               "pose) and export glTF 2.0 (.glb) with a texture; then Import model….");
    port::platform::openURL(("file://" + skins::folder()).c_str());
}

void picked(bool texture, const std::string& path) {
    if (texture) importTexture(path);
    else importModel(path);
}

void reloadSkins() {
    skins::rescan();
    s_choices = skins::list();
    s_reload = true;
}

void shutdown() {
    for (auto& t : s_textures) SDL_DestroyTexture(t.second);
    s_textures.clear();
}

#ifdef __ANDROID__
bool previewRequested() { return s_previewPending; }

bool takePreviewRequest(std::vector<std::string>* env) {
    if (!s_previewPending) return false;
    s_previewPending = false;
    *env = s_previewEnv;
    return true;
}
#endif

}
}
}
