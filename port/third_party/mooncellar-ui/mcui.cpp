#include "mcui.h"

#include <cstdio>

#include "imgui_internal.h"

namespace mcui {

using namespace tokens;

namespace {

Fonts s_fonts;
float s_scale = 1.0f;

bool fileExists(const std::string& path) {
    if (path.empty()) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

// The face, then every fallback merged into it for the glyphs it lacks
ImFont* addFace(const std::string& file, const std::vector<FontFile>& fallback) {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.Flags |= ImFontFlags_NoLoadError;
    ImFont* font = fileExists(file) ? io.Fonts->AddFontFromFileTTF(file.c_str(), kTextBody, &cfg) : nullptr;
    for (const FontFile& extra : fallback) {
        if (!fileExists(extra.path)) continue;
        ImFontConfig merge;
        merge.Flags |= ImFontFlags_NoLoadError;
        merge.MergeMode = font != nullptr;
        merge.FontNo = extra.index;
        ImFont* added = io.Fonts->AddFontFromFileTTF(extra.path.c_str(), kTextBody, &merge);
        if (!font) font = added;
    }
    return font;
}

// The site darkens a hovered button (filter: brightness(0.8))
ImU32 darker(unsigned token) {
    ImVec4 c = Color(token);
    return ImGui::GetColorU32(ImVec4(c.x * 0.8f, c.y * 0.8f, c.z * 0.8f, c.w));
}

// --easing-out, cubic-bezier(0.16, 1, 0.3, 1): close to an exponential ease-out
float easeOut(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    return 1.0f - ImPow(1.0f - t, 4.0f);
}

}  // namespace

ImVec4 Color(unsigned token, float alpha) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(token);
    c.w *= alpha;
    return c;
}

ImU32 Packed(unsigned token, float alpha) { return ImGui::ColorConvertFloat4ToU32(Color(token, alpha)); }

const Fonts& LoadFonts(const FontFiles& files) {
    s_fonts = Fonts();
    s_fonts.regular = addFace(files.regular, files.fallback);
    s_fonts.brand = s_fonts.regular && fileExists(files.regular);
    if (!s_fonts.regular) s_fonts.regular = ImGui::GetIO().Fonts->AddFontDefault();
    s_fonts.medium = addFace(files.medium, files.fallback);
    if (!s_fonts.medium) s_fonts.medium = s_fonts.regular;
    s_fonts.bold = addFace(files.bold, files.fallback);
    if (!s_fonts.bold) s_fonts.bold = s_fonts.medium;
    ImGui::GetIO().FontDefault = s_fonts.regular;
    return s_fonts;
}

const Fonts& GetFonts() { return s_fonts; }

float Scale() { return s_scale; }

void ApplyStyle(float scale) {
    s_scale = scale;
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();
    ImGui::StyleColorsDark(&s);
    s.WindowPadding = ImVec2(kPaddingX6, kPaddingX5);
    s.FramePadding = ImVec2(kPaddingX4, kPaddingX2);
    s.CellPadding = ImVec2(kPaddingX2, kPaddingX1);
    s.ItemSpacing = ImVec2(kGapX3, kGapX3);
    s.ItemInnerSpacing = ImVec2(kGapX2, kGapX2);
    s.IndentSpacing = kPaddingX5;
    s.ScrollbarSize = kPaddingX3;
    s.GrabMinSize = kPaddingX3;
    s.WindowRounding = 0.0f;
    s.ChildRounding = kRadiusX5;
    s.FrameRounding = kRadiusControl;
    s.PopupRounding = kRadiusX3;
    s.ScrollbarRounding = kRadiusX4;
    s.GrabRounding = kRadiusX4;
    s.TabRounding = kRadiusX3;
    s.WindowBorderSize = 0.0f;
    s.ChildBorderSize = 0.0f;
    s.PopupBorderSize = 1.0f;
    s.FrameBorderSize = 0.0f;
    s.TabBorderSize = 0.0f;
    s.TabBarBorderSize = 0.0f;
    s.TabBarOverlineSize = 0.0f;
    s.SeparatorTextBorderSize = 1.0f;
    s.SeparatorTextPadding = ImVec2(kPaddingX4, kPaddingX1);
    s.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    s.ScaleAllSizes(scale);
    // The text size stays the font's own (LoadFonts loads at kTextBody), so a window that loads its
    // own font keeps its size
    s.FontScaleDpi = scale;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = Color(kColorTextPrimary);
    c[ImGuiCol_TextDisabled] = Color(kColorTextMuted);
    c[ImGuiCol_WindowBg] = Color(kColorBgPrimary);
    c[ImGuiCol_ChildBg] = Color(kColorBgSecondary);
    c[ImGuiCol_PopupBg] = Color(kColorBgPrimary);
    c[ImGuiCol_Border] = Color(kColorBorderPrimary);
    c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    // Fields step back to the page colour inside a panel
    c[ImGuiCol_FrameBg] = Color(kColorBgPrimary);
    c[ImGuiCol_FrameBgHovered] = Color(kColorBgHover);
    c[ImGuiCol_FrameBgActive] = Color(kColorBgHover);
    c[ImGuiCol_TitleBg] = Color(kColorBgPrimary);
    c[ImGuiCol_TitleBgActive] = Color(kColorBgPrimary);
    c[ImGuiCol_TitleBgCollapsed] = Color(kColorBgPrimary);
    c[ImGuiCol_MenuBarBg] = Color(kColorBgSecondary);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = Color(kColorBgHover);
    c[ImGuiCol_ScrollbarGrabHovered] = Color(kColorTextMuted, 0.6f);
    c[ImGuiCol_ScrollbarGrabActive] = Color(kColorTextMuted);
    c[ImGuiCol_CheckMark] = Color(kColorAccent);
    c[ImGuiCol_SliderGrab] = Color(kColorAccent);
    c[ImGuiCol_SliderGrabActive] = Color(kColorAccent);
    c[ImGuiCol_Button] = Color(kColorBgTertiary);
    c[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(darker(kColorBgTertiary));
    c[ImGuiCol_ButtonActive] = Color(kColorBgHover);
    c[ImGuiCol_Header] = Color(kColorBgTertiary);
    c[ImGuiCol_HeaderHovered] = Color(kColorBgHover);
    c[ImGuiCol_HeaderActive] = Color(kColorBgHover);
    c[ImGuiCol_Separator] = Color(kColorBgTertiary);
    c[ImGuiCol_SeparatorHovered] = Color(kColorAccent);
    c[ImGuiCol_SeparatorActive] = Color(kColorAccent);
    c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered] = Color(kColorAccent, 0.6f);
    c[ImGuiCol_ResizeGripActive] = Color(kColorAccent);
    c[ImGuiCol_InputTextCursor] = Color(kColorTextPrimary);
    // Tabs: the site's segmented control — tertiary track, the chosen one on the panel colour
    c[ImGuiCol_Tab] = Color(kColorBgTertiary);
    c[ImGuiCol_TabHovered] = Color(kColorBgHover);
    c[ImGuiCol_TabSelected] = Color(kColorBgSecondary);
    c[ImGuiCol_TabSelectedOverline] = Color(kColorAccent);
    c[ImGuiCol_TabDimmed] = Color(kColorBgTertiary);
    c[ImGuiCol_TabDimmedSelected] = Color(kColorBgSecondary);
    c[ImGuiCol_TabDimmedSelectedOverline] = Color(kColorAccent);
    c[ImGuiCol_PlotLines] = Color(kColorTextSecondary);
    c[ImGuiCol_PlotLinesHovered] = Color(kColorAccent);
    c[ImGuiCol_PlotHistogram] = Color(kColorAccent);
    c[ImGuiCol_PlotHistogramHovered] = Color(kColorAccent);
    c[ImGuiCol_TableHeaderBg] = Color(kColorBgTertiary);
    c[ImGuiCol_TableBorderStrong] = Color(kColorBgTertiary);
    c[ImGuiCol_TableBorderLight] = Color(kColorBgTertiary);
    c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);
    c[ImGuiCol_TextLink] = Color(kColorAccent);
    c[ImGuiCol_TextSelectedBg] = Color(kColorAccent, 0.35f);
    c[ImGuiCol_DragDropTarget] = Color(kColorAccent);
    c[ImGuiCol_NavCursor] = Color(kColorAccent);
    c[ImGuiCol_NavWindowingHighlight] = Color(kColorAccent);
    c[ImGuiCol_NavWindowingDimBg] = Color(kColorBgPrimary, 0.6f);
    c[ImGuiCol_ModalWindowDimBg] = Color(kColorBgPrimary, 0.6f);
}

void Title(const char* text) {
    ImGui::PushFont(s_fonts.bold, kTextTitle);
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
}

void Caption(const char* text) {
    ImGui::PushFont(s_fonts.regular, kTextCaption);
    ImGui::PushStyleColor(ImGuiCol_Text, Color(kColorTextMuted));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void SectionTitle(const char* text) {
    if (ImGui::GetCursorPosY() > ImGui::GetCursorStartPos().y + 1.0f) ImGui::Dummy(ImVec2(0, kGapX2 * s_scale));
    ImGui::PushFont(s_fonts.medium, kTextBody + 1.0f);
    float h = ImGui::GetTextLineHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float bar = 4.0f * s_scale;
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + bar, p.y + h), Packed(kColorAccent), kRadiusX2 * s_scale);
    ImGui::SetCursorScreenPos(ImVec2(p.x + bar + kGapX2 * s_scale, p.y));
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
}

bool Button(const char* label, ButtonKind kind, ImVec2 size) {
    unsigned fill = kColorBgTertiary, text = kColorTextPrimary;
    switch (kind) {
    case ButtonKind::Default: break;
    case ButtonKind::Accent: fill = kColorAccent; break;
    case ButtonKind::Danger: fill = kColorNegative; break;
    case ButtonKind::Ghost: break;
    }
    ImVec4 base = kind == ButtonKind::Ghost ? ImVec4(0, 0, 0, 0) : Color(fill);
    ImVec4 hovered = kind == ButtonKind::Ghost ? Color(kColorBgHover) : ImGui::ColorConvertU32ToFloat4(darker(fill));
    ImGui::PushStyleColor(ImGuiCol_Button, base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, hovered);
    ImGui::PushStyleColor(ImGuiCol_Text, Color(text));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, kRadiusButton * s_scale);
    ImGui::PushFont(s_fonts.medium, 0.0f);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    return pressed;
}

bool Toggle(const char* label, bool* value) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const float h = ImGui::GetFrameHeight();
    const float w = h * 2.0f;
    const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
    const ImVec2 pos = window->DC.CursorPos;
    const float labelWidth = labelSize.x > 0 ? g.Style.ItemInnerSpacing.x + labelSize.x : 0.0f;
    const ImRect bb(pos, ImVec2(pos.x + w + labelWidth, pos.y + h));
    ImGui::ItemSize(bb, g.Style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *value = !*value;
        ImGui::MarkItemEdited(id);
    }
    // The knob slides over --duration-base
    float* t = window->StateStorage.GetFloatRef(id, *value ? 1.0f : 0.0f);
    float target = *value ? 1.0f : 0.0f;
    float step = g.IO.DeltaTime / kDurationBase;
    *t = *t < target ? ImMin(*t + step, target) : ImMax(*t - step, target);
    const float eased = easeOut(*value ? *t : 1.0f - *t);
    const float k = *value ? eased : 1.0f - eased;

    ImDrawList* dl = window->DrawList;
    const ImRect track(pos, ImVec2(pos.x + w, pos.y + h));
    const float r = h * 0.5f;
    const unsigned state = *value ? kColorPositive : kColorNegative;
    dl->AddRectFilled(track.Min, track.Max, Packed(hovered ? kColorBgHover : kColorBgPrimary), r);
    dl->AddRect(track.Min, track.Max, Packed(state, 0.85f), r, 0, 2.0f * s_scale);
    const float inset = 4.0f * s_scale;
    const float knobW = (w - inset * 2) * 0.5f;
    const float x = track.Min.x + inset + k * knobW;
    dl->AddRectFilled(ImVec2(x, track.Min.y + inset), ImVec2(x + knobW, track.Max.y - inset), Packed(state),
                      r - inset);
    ImGui::RenderNavCursor(track, id);
    if (labelSize.x > 0) {
        ImGui::RenderText(ImVec2(track.Max.x + g.Style.ItemInnerSpacing.x, pos.y + g.Style.FramePadding.y), label);
    }
    return pressed;
}

bool Segmented(const char* id, const char* const* items, int count, int* current, bool gamepadShoulders) {
    ImGui::PushID(id);
    bool changed = false;
    bool focusChosen = false;
    if (gamepadShoulders && count > 1 && !ImGui::IsAnyItemActive() &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
        int step = ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)   ? -1
                   : ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) ? 1
                                                                     : 0;
        if (step != 0) {
            *current = (*current + step + count) % count;
            changed = focusChosen = true;
        }
    }
    // Padding and gap of one step: the selected segment never touches the track or its neighbours, and
    // the track's radius (x4) is the segment's (x3) plus the padding, so the corners stay concentric
    const float pad = kPaddingX1 * s_scale;
    const float gap = kPaddingX1 * s_scale;
    const float h = ImGui::GetFrameHeight();
    const ImGuiStyle& style = ImGui::GetStyle();
    float width = pad * 2 + gap * (count > 1 ? count - 1 : 0);
    for (int i = 0; i < count; i++) width += ImGui::CalcTextSize(items[i]).x + style.FramePadding.x * 2;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h + pad * 2), Packed(kColorBgTertiary), kRadiusX4 * s_scale);
    float x = p.x + pad;
    ImGui::PushFont(s_fonts.medium, 0.0f);
    for (int i = 0; i < count; i++) {
        float w = ImGui::CalcTextSize(items[i]).x + style.FramePadding.x * 2;
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + pad));
        ImGui::PushID(i);
        bool chosen = *current == i;
        if (ImGui::InvisibleButton("##item", ImVec2(w, h), ImGuiButtonFlags_EnableNav) && !chosen) {
            *current = i;
            changed = true;
        }
        if (focusChosen && chosen) ImGui::SetKeyboardFocusHere(-1);
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (chosen || hovered) {
            dl->AddRectFilled(ImVec2(x, p.y + pad), ImVec2(x + w, p.y + pad + h),
                              Packed(chosen ? kColorBgSecondary : kColorBgHover), kRadiusX3 * s_scale);
        }
        ImVec2 ts = ImGui::CalcTextSize(items[i]);
        dl->AddText(ImVec2(x + (w - ts.x) * 0.5f, p.y + pad + (h - ts.y) * 0.5f),
                    Packed(chosen ? kColorAccent : kColorTextPrimary), items[i]);
        x += w + gap;
    }
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(p);
    ImGui::Dummy(ImVec2(width, h + pad * 2));
    ImGui::PopID();
    return changed;
}

void ProgressBar(float fraction, ImVec2 size, unsigned color) {
    float w = size.x < 0 ? ImGui::GetContentRegionAvail().x : size.x;
    float h = size.y > 0 ? size.y : 6.0f * s_scale;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float r = kRadiusX1 * s_scale;
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), Packed(kColorBgPrimary), r);
    float f = ImClamp(fraction, 0.0f, 1.0f);
    if (f > 0) dl->AddRectFilled(p, ImVec2(p.x + ImMax(w * f, h), p.y + h), Packed(color), r);
    ImGui::Dummy(ImVec2(w, h));
}

bool BeginPanel(const char* id, ImVec2 size) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(kColorBgSecondary, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, kRadiusX5 * s_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(kPaddingX5 * s_scale, kPaddingX4 * s_scale));
    ImGuiChildFlags flags = ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened;
    if (size.y == 0) flags |= ImGuiChildFlags_AutoResizeY;
    bool open = ImGui::BeginChild(id, size, flags);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    return open;
}

void EndPanel() { ImGui::EndChild(); }

}  // namespace mcui
