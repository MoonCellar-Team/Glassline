#pragma once

#include <algorithm>

#include "imgui.h"
#include "imgui_internal.h"
#include "mcui.h"

namespace port::platform {

inline float launcherHelpButtonSize() { return std::max(28.0f, ImGui::GetFrameHeight()); }

inline const mcui::Icon& launcherInfoIcon() {
    static const mcui::Icon icon = [] {
        mcui::Icon result = mcui::ParseIcon(
            R"(<path d="M12 2a10 10 0 1 0 0 20a10 10 0 1 0 0-20Z"/><path d="M12 16v-4"/><path d="M12 8h.01"/>)");
        for (auto& stroke : result.strokes) {
            if (!stroke.closed) continue;
            while (stroke.points.size() > 1 && stroke.points.front().x == stroke.points.back().x &&
                   stroke.points.front().y == stroke.points.back().y)
                stroke.points.pop_back();
        }
        return result;
    }();
    return icon;
}

inline const mcui::Icon& launcherHelpCloseIcon() {
    static const mcui::Icon icon = [] {
        mcui::Icon result = mcui::ParseIcon(R"(<path d="M9 9l6 6"/><path d="M15 9l-6 6"/>)");
        result.strokes.insert(result.strokes.begin(), launcherInfoIcon().strokes.front());
        return result;
    }();
    return icon;
}

template <class Content>
inline void launcherHelpContent(const char* id, Content content, bool inlineButton = true, bool open = false) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float size = launcherHelpButtonSize();
    const ImVec2 previousMin = ImGui::GetItemRectMin(), previousMax = ImGui::GetItemRectMax();
    if (inlineButton && ImGui::GetItemRectMax().x + style.ItemSpacing.x + size <= ImGui::GetCurrentWindow()->WorkRect.Max.x)
        ImGui::SameLine();
    if (ImGui::GetCurrentWindow()->DC.IsSameLine)
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, (previousMin.y + previousMax.y - size) / 2));
    const bool disabled = (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (disabled) ImGui::BeginDisabledOverrideReenable();
    ImGui::PushID(id);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton("##info", ImVec2(size, size), ImGuiButtonFlags_EnableNav);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 center(at.x + size / 2, at.y + size / 2);
    const ImU32 ink = ImGui::GetColorU32(hovered || ImGui::IsItemFocused() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    if (hovered || ImGui::IsItemFocused()) draw->AddCircleFilled(center, size * .46f, ImGui::GetColorU32(ImGuiCol_FrameBgHovered));
    mcui::DrawIcon(draw, launcherInfoIcon(), ImVec2(center.x - size * .3f, center.y - size * .3f), size * .6f, ink);
    if (pressed || open) ImGui::OpenPopup("##help");
    const bool pinned = ImGui::IsPopupOpen("##help");
    const bool showHover = hovered && ImGui::GetIO().MouseSource == ImGuiMouseSource_Mouse;
    if (!pinned && !showHover) {
        ImGui::PopID();
        if (disabled) ImGui::EndDisabledOverrideReenable();
        return;
    }
    const ImVec2 view = ImGui::GetIO().DisplaySize;
    const float margin = 8.0f, gap = 6.0f * mcui::Scale();
    const float width = std::max(1.0f, std::min(420.0f * mcui::Scale(), view.x - margin * 2));
    const float bodyWidth = std::max(1.0f, width - style.WindowPadding.x * 2 - (pinned ? size + style.ItemSpacing.x : 0));
    ImGuiStorage* cache = ImGui::GetStateStorage();
    const ImGuiID heightId = ImGui::GetID(pinned ? "##popup-height" : "##hover-height");
    const ImGuiID widthId = ImGui::GetID(pinned ? "##popup-width" : "##hover-width");
    const bool measured = cache->GetFloat(heightId, -1.0f) >= 0;
    float measuredHeight = cache->GetFloat(heightId, size);
    const float previousWidth = cache->GetFloat(widthId, bodyWidth);
    measuredHeight *= previousWidth / bodyWidth;
    const float minHeight = style.WindowPadding.y * 2 + (pinned ? size : 0);
    const float desiredHeight = style.WindowPadding.y * 2 + std::max(pinned ? size : 0.0f, measuredHeight);
    const float below = std::max(0.0f, view.y - margin - (at.y + size + gap));
    const float above = std::max(0.0f, at.y - gap - margin);
    const bool placeAbove = desiredHeight > below && above > below;
    const float maxHeight = std::min(view.y - margin * 2, std::max(minHeight, placeAbove ? above : below));
    const float height = std::min(desiredHeight, maxHeight);
    const float x = std::max(margin, std::min(center.x - width / 2, view.x - width - margin));
    const float y = std::max(margin, std::min(placeAbove ? at.y - gap - height : at.y + size + gap, view.y - margin - height));
    auto bounds = [&] {
        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::SetNextWindowSize(ImVec2(width, measured ? height : 0));
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, minHeight), ImVec2(width, maxHeight));
    };
    auto body = [&] {
        ImGui::PushStyleColor(ImGuiCol_Text, mcui::Color(mcui::tokens::kColorTextPrimary));
        ImGui::PushTextWrapPos(0);
        content();
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    };
    auto measure = [&] {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        cache->SetFloat(heightId, std::max(0.0f, window->DC.CursorMaxPos.y - window->DC.CursorStartPos.y));
        cache->SetFloat(widthId, bodyWidth);
    };
    bounds();
    if (pinned && ImGui::BeginPopup("##help", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const ImVec2 popupAt = ImGui::GetWindowPos();
        const float closeInset = 6.0f * mcui::Scale();
        const ImVec2 closeAt(popupAt.x + ImGui::GetWindowWidth() - closeInset - size, popupAt.y + closeInset);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, size), ImVec2(bodyWidth, std::max(size, maxHeight - style.WindowPadding.y * 2)));
        if (ImGui::BeginChild("##help-body", ImVec2(bodyWidth, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings)) {
            body();
            measure();
            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseSource == ImGuiMouseSource_TouchScreen && io.MouseDown[0] &&
                ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                const ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
                if (drag.y * drag.y > 64.0f && drag.y * drag.y > drag.x * drag.x) {
                    ImGui::ClearActiveID();
                    ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
                }
            }
        }
        ImGui::EndChild();
        ImGui::SetCursorScreenPos(closeAt);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, size / 2);
        if (ImGui::InvisibleButton("##close", ImVec2(size, size), ImGuiButtonFlags_EnableNav)) ImGui::CloseCurrentPopup();
        ImGui::PopStyleVar();
        ImDrawList* popupDraw = ImGui::GetWindowDrawList();
        const ImVec2 closeCenter(closeAt.x + size / 2, closeAt.y + size / 2);
        if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
            popupDraw->AddCircleFilled(closeCenter, size * .46f, ImGui::GetColorU32(mcui::Color(mcui::tokens::kColorNegative, .18f)));
        mcui::DrawIcon(popupDraw, launcherHelpCloseIcon(), ImVec2(closeCenter.x - size * .3f, closeCenter.y - size * .3f),
                       size * .6f, ImGui::GetColorU32(mcui::Color(mcui::tokens::kColorNegative)));
        ImGui::EndPopup();
    } else if (!pinned && showHover) {
        bounds();
        ImGui::BeginTooltip();
        body();
        measure();
        ImGui::EndTooltip();
    }
    ImGui::PopID();
    if (disabled) ImGui::EndDisabledOverrideReenable();
}

inline void launcherHelp(const char* text, bool inlineButton = true) {
    launcherHelpContent(text, [text] { ImGui::TextWrapped("%s", text); }, inlineButton);
}

}
