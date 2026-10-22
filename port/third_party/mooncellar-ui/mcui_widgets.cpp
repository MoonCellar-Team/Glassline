// Composite components: the achievement card, the device-code sign-in and the notifications.
#include <deque>
#include <mutex>

#include "imgui_internal.h"
#include "mcui.h"

namespace mcui {

using namespace tokens;

namespace {

// Wrapped text straight to a draw list; returns its height
float drawText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 color, const std::string& text,
               float wrap) {
    if (text.empty()) return 0.0f;
    dl->AddText(font, size, pos, color, text.c_str(), nullptr, wrap);
    return font->CalcTextSizeA(size, FLT_MAX, wrap, text.c_str()).y;
}

float textHeight(ImFont* font, float size, const std::string& text, float wrap) {
    return text.empty() ? 0.0f : font->CalcTextSizeA(size, FLT_MAX, wrap, text.c_str()).y;
}

float easeOut(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    return 1.0f - ImPow(1.0f - t, 4.0f);
}

}  // namespace

void AchievementCard(const AchievementView& view, float width) {
    const float s = Scale();
    const Fonts& fonts = GetFonts();
    if (width <= 0) width = ImGui::GetContentRegionAvail().x;
    const float pad = kPaddingX3 * s, tile = 48.0f * s, gap = kGapX3 * s;
    const float textX = pad + tile + gap, wrap = ImMax(1.0f, width - textX - pad);
    const float title = kTextBody * s, body = kTextCaption * s;
    const bool progress = view.target > 0 && view.current >= 0 && !view.unlocked;
    float h = textHeight(fonts.bold, title, view.title, wrap) + kGapX1 * s +
              textHeight(fonts.regular, body, view.description, wrap);
    if (progress) h += kGapX2 * s + 6.0f * s;
    if (!view.status.empty()) h += kGapX2 * s + textHeight(fonts.regular, body, view.status, wrap);
    h = ImMax(h, tile) + pad * 2;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), Packed(kColorBgTertiary), kRadiusX3 * s);
    // The tile: accent when unlocked, the page colour and a muted icon when not
    ImVec2 t0(p.x + pad, p.y + pad);
    dl->AddRectFilled(t0, ImVec2(t0.x + tile, t0.y + tile), Packed(view.unlocked ? kColorAccent : kColorBgPrimary),
                      kRadiusX3 * s);
    if (view.icon && view.icon->Valid()) {
        float icon = tile * 0.58f;
        DrawIcon(dl, *view.icon, ImVec2(t0.x + (tile - icon) / 2, t0.y + (tile - icon) / 2), icon,
                 Packed(view.unlocked ? kColorTextPrimary : kColorTextMuted));
    }
    float y = p.y + pad;
    y += drawText(dl, fonts.bold, title, ImVec2(p.x + textX, y),
                  Packed(view.unlocked ? kColorTextPrimary : kColorTextSecondary), view.title, wrap);
    y += kGapX1 * s;
    y += drawText(dl, fonts.regular, body, ImVec2(p.x + textX, y), Packed(kColorTextMuted), view.description, wrap);
    if (progress) {
        y += kGapX2 * s;
        float barW = wrap, barH = 6.0f * s, r = kRadiusX1 * s;
        float f = ImClamp(float(view.current) / float(view.target), 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2(p.x + textX, y), ImVec2(p.x + textX + barW, y + barH), Packed(kColorBgPrimary), r);
        if (f > 0)
            dl->AddRectFilled(ImVec2(p.x + textX, y), ImVec2(p.x + textX + ImMax(barW * f, barH), y + barH),
                              Packed(kColorAccent), r);
        y += barH;
    }
    if (!view.status.empty()) {
        y += kGapX2 * s;
        drawText(dl, fonts.regular, body, ImVec2(p.x + textX, y),
                 Packed(view.unlocked ? kColorPositive : kColorTextMuted), view.status, wrap);
    }
    ImGui::Dummy(ImVec2(width, h));
}

void DeviceCode(const char* code, const char* url) {
    const float s = Scale();
    const Fonts& fonts = GetFonts();
    ImGui::PushFont(fonts.bold, kTextDisplay);
    ImVec2 size = ImGui::CalcTextSize(code);
    ImGui::PopFont();
    const float padX = kPaddingX5 * s, padY = kPaddingX3 * s;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 box(size.x + padX * 2, size.y + padY * 2);
    dl->AddRectFilled(p, ImVec2(p.x + box.x, p.y + box.y), Packed(kColorBgPrimary), kRadiusControl * s);
    dl->AddRect(p, ImVec2(p.x + box.x, p.y + box.y), Packed(kColorAccent), kRadiusControl * s, 0, 2.0f * s);
    dl->AddText(fonts.bold, kTextDisplay * s, ImVec2(p.x + padX, p.y + padY), Packed(kColorTextPrimary), code);
    ImGui::Dummy(box);
    if (!url || !*url) return;
    ImGui::PushFont(fonts.regular, kTextCaption);
    ImGui::PushStyleColor(ImGuiCol_Text, Color(kColorTextMuted));
    ImGui::TextUnformatted(url);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

// ---- Notifications ----

namespace {

struct Entry {
    Notification n;
    double shown = -1;  // when it came on screen
};

std::mutex s_mutex;
std::deque<Entry> s_queue;
ImTextureID s_mark = ImTextureID_Invalid;
constexpr int kVisible = 3;

}  // namespace

void Notify(const Notification& notification) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_queue.push_back(Entry{notification, -1});
}

void SetNotificationMark(ImTextureID mark) { s_mark = mark; }

bool NotificationsActive() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return !s_queue.empty();
}

void DrawNotifications(double now) {
    std::deque<Entry> visible;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        // Gone: past their time plus the exit
        while (!s_queue.empty() && s_queue.front().shown >= 0 &&
               now - s_queue.front().shown > s_queue.front().n.seconds + kDurationFast)
            s_queue.pop_front();
        for (size_t i = 0; i < s_queue.size() && int(i) < kVisible; i++) {
            if (s_queue[i].shown < 0) s_queue[i].shown = now;
            visible.push_back(s_queue[i]);
        }
    }
    if (visible.empty()) return;
    const float s = Scale();
    const Fonts& fonts = GetFonts();
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
    const float width = ImMin(360.0f * s, vp->Size.x - kPaddingX4 * s * 2);
    const float pad = kPaddingX4 * s, tile = 40.0f * s, gap = kGapX3 * s, mark = 16.0f * s;
    const float textX = pad + tile + gap, wrap = ImMax(1.0f, width - textX - pad - mark - kGapX2 * s);
    const float titleSize = kTextBody * s, bodySize = kTextCaption * s;
    float y = vp->Pos.y + kPaddingX4 * s;
    for (const Entry& e : visible) {
        const Notification& n = e.n;
        const float age = float(now - e.shown);
        // In: slides from the right over --duration-base; out: fades over --duration-fast
        float in = easeOut(age / kDurationBase);
        float out = age > n.seconds ? 1.0f - ImClamp((age - n.seconds) / kDurationFast, 0.0f, 1.0f) : 1.0f;
        float alpha = ImMin(in, out);
        float h = textHeight(fonts.bold, titleSize, n.title, wrap) + kGapX1 * s +
                  textHeight(fonts.regular, bodySize, n.text, wrap);
        h = ImMax(h, tile) + pad * 2;
        float x = vp->Pos.x + vp->Size.x - width - kPaddingX4 * s + (1.0f - in) * (width * 0.25f);
        ImVec2 p(x, y), q(x + width, y + h);
        // The site's Toast: --color-bg-primary, --radius-x2, a shadow; the time left — an accent line
        dl->AddRectFilled(ImVec2(p.x, p.y + 2 * s), ImVec2(q.x, q.y + 4 * s), IM_COL32(0, 0, 0, int(90 * alpha)),
                          kRadiusX2 * s);
        dl->AddRectFilled(p, q, Packed(kColorBgPrimary, alpha), kRadiusX2 * s);
        dl->AddRect(p, q, Packed(kColorBgTertiary, alpha), kRadiusX2 * s);
        // The tile: accent under an achievement's icon, tertiary under the mark (its violet would vanish)
        ImVec2 t0(p.x + pad, p.y + pad);
        const bool hasIcon = n.icon && n.icon->Valid();
        dl->AddRectFilled(t0, ImVec2(t0.x + tile, t0.y + tile), Packed(hasIcon ? kColorAccent : kColorBgTertiary, alpha),
                          kRadiusX3 * s);
        if (hasIcon) {
            float icon = tile * 0.6f;
            DrawIcon(dl, *n.icon, ImVec2(t0.x + (tile - icon) / 2, t0.y + (tile - icon) / 2), icon,
                     Packed(kColorTextPrimary, alpha));
        } else if (s_mark != ImTextureID_Invalid) {
            float m = tile * 0.62f;
            ImVec2 m0(t0.x + (tile - m) / 2, t0.y + (tile - m) / 2);
            dl->AddImage(ImTextureRef(s_mark), m0, ImVec2(m0.x + m, m0.y + m), ImVec2(0, 0), ImVec2(1, 1),
                         IM_COL32(255, 255, 255, int(255 * alpha)));
        }
        float ty = p.y + pad;
        ty += drawText(dl, fonts.bold, titleSize, ImVec2(p.x + textX, ty), Packed(kColorTextPrimary, alpha), n.title,
                       wrap);
        ty += kGapX1 * s;
        drawText(dl, fonts.regular, bodySize, ImVec2(p.x + textX, ty), Packed(kColorGray, alpha), n.text, wrap);
        // The MoonCellar mark in the corner when the tile carries the achievement's icon
        if (n.icon && n.icon->Valid() && s_mark != ImTextureID_Invalid) {
            ImVec2 m0(q.x - pad - mark, p.y + pad);
            dl->AddImage(ImTextureRef(s_mark), m0, ImVec2(m0.x + mark, m0.y + mark), ImVec2(0, 0), ImVec2(1, 1),
                         IM_COL32(255, 255, 255, int(255 * alpha)));
        }
        float left = 1.0f - ImClamp(age / ImMax(n.seconds, 0.1f), 0.0f, 1.0f);
        float lineY = q.y - 5.0f * s;
        float lineX0 = p.x + pad, lineX1 = q.x - pad;
        dl->AddLine(ImVec2(lineX0, lineY), ImVec2(lineX0 + (lineX1 - lineX0) * left, lineY), Packed(kColorAccent, alpha),
                    2.0f * s);
        y = q.y + kGapX3 * s;
    }
}

}  // namespace mcui
