// Outline icons: the `d` of every <path> of a 24×24 SVG, flattened into polylines once and drawn
// with ImGui's lines. The format is the one of the ports' achievement icons and the site's icons
// (stroke 1.8, round caps and joins), so the caps and the corners get a round dot.
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "imgui_internal.h"
#include "mcui.h"

namespace mcui {

namespace {

constexpr float kViewBox = 24.0f;
constexpr float kStroke = 1.8f;
constexpr int kCurveSteps = 10;

class PathParser {
public:
    explicit PathParser(const std::string& d) : s_(d) {}

    void parse(Icon* icon) {
        char cmd = 0;
        while (skip(), i_ < s_.size()) {
            if (std::isalpha(static_cast<unsigned char>(s_[i_]))) cmd = s_[i_++];
            else if (!cmd) return;  // numbers before any command: not a path
            if (!command(cmd, icon)) return;
            // After M the repeats are L; others repeat themselves
            if (cmd == 'M') cmd = 'L';
            else if (cmd == 'm') cmd = 'l';
            if (cmd == 'Z' || cmd == 'z') cmd = 0;
        }
        finish(icon);
    }

private:
    const std::string& s_;
    size_t i_ = 0;
    ImVec2 cur_{0, 0}, start_{0, 0}, ctrl_{0, 0};
    char last_ = 0;
    Icon::Stroke stroke_;

    void skip() {
        while (i_ < s_.size() && (std::isspace(static_cast<unsigned char>(s_[i_])) || s_[i_] == ',')) i_++;
    }

    bool number(float* out) {
        skip();
        size_t j = i_;
        if (j < s_.size() && (s_[j] == '-' || s_[j] == '+')) j++;
        bool digits = false, dot = false;
        while (j < s_.size()) {
            char c = s_[j];
            if (std::isdigit(static_cast<unsigned char>(c))) digits = true;
            else if (c == '.' && !dot) dot = true;
            else break;
            j++;
        }
        if (digits && j < s_.size() && (s_[j] == 'e' || s_[j] == 'E')) {
            size_t k = j + 1;
            if (k < s_.size() && (s_[k] == '-' || s_[k] == '+')) k++;
            if (k < s_.size() && std::isdigit(static_cast<unsigned char>(s_[k]))) {
                while (k < s_.size() && std::isdigit(static_cast<unsigned char>(s_[k]))) k++;
                j = k;
            }
        }
        if (!digits) return false;
        *out = std::strtof(s_.substr(i_, j - i_).c_str(), nullptr);
        i_ = j;
        return true;
    }

    bool flag(bool* out) {
        skip();
        if (i_ < s_.size() && (s_[i_] == '0' || s_[i_] == '1')) {
            *out = s_[i_++] == '1';
            return true;
        }
        return false;
    }

    bool point(bool relative, ImVec2* p) {
        float x, y;
        if (!number(&x) || !number(&y)) return false;
        *p = relative ? ImVec2(cur_.x + x, cur_.y + y) : ImVec2(x, y);
        return true;
    }

    void finish(Icon* icon) {
        if (!stroke_.points.empty()) {
            if (!stroke_.closed) {
                stroke_.joints.push_back(stroke_.points.front());
                stroke_.joints.push_back(stroke_.points.back());
            }
            icon->strokes.push_back(stroke_);
        }
        stroke_ = Icon::Stroke();
    }

    void lineTo(ImVec2 p) {
        if (stroke_.points.empty()) stroke_.points.push_back(cur_);
        stroke_.points.push_back(p);
        cur_ = p;
    }

    void corner() { stroke_.joints.push_back(cur_); }

    void cubic(ImVec2 c1, ImVec2 c2, ImVec2 end) {
        ImVec2 p0 = cur_;
        for (int k = 1; k <= kCurveSteps; k++) {
            float t = float(k) / kCurveSteps, u = 1 - t;
            lineTo(ImVec2(u * u * u * p0.x + 3 * u * u * t * c1.x + 3 * u * t * t * c2.x + t * t * t * end.x,
                          u * u * u * p0.y + 3 * u * u * t * c1.y + 3 * u * t * t * c2.y + t * t * t * end.y));
        }
        ctrl_ = c2;
    }

    void quad(ImVec2 c, ImVec2 end) {
        ImVec2 p0 = cur_;
        for (int k = 1; k <= kCurveSteps; k++) {
            float t = float(k) / kCurveSteps, u = 1 - t;
            lineTo(ImVec2(u * u * p0.x + 2 * u * t * c.x + t * t * end.x, u * u * p0.y + 2 * u * t * c.y + t * t * end.y));
        }
        ctrl_ = c;
    }

    // SVG elliptical arc, endpoint to centre parameterisation (SVG 1.1, F.6.5)
    void arc(float rx, float ry, float rotation, bool large, bool sweep, ImVec2 end) {
        ImVec2 p0 = cur_;
        if ((p0.x == end.x && p0.y == end.y)) return;
        rx = std::fabs(rx);
        ry = std::fabs(ry);
        if (rx == 0 || ry == 0) {
            lineTo(end);
            return;
        }
        const float phi = rotation * IM_PI / 180.0f, cp = std::cos(phi), sp = std::sin(phi);
        const float dx = (p0.x - end.x) / 2, dy = (p0.y - end.y) / 2;
        const float x1 = cp * dx + sp * dy, y1 = -sp * dx + cp * dy;
        float lambda = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
        if (lambda > 1) {
            float k = std::sqrt(lambda);
            rx *= k;
            ry *= k;
        }
        float num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
        float den = rx * rx * y1 * y1 + ry * ry * x1 * x1;
        float coef = den > 0 ? std::sqrt(ImMax(0.0f, num / den)) : 0;
        if (large == sweep) coef = -coef;
        const float cx1 = coef * rx * y1 / ry, cy1 = -coef * ry * x1 / rx;
        const float cx = cp * cx1 - sp * cy1 + (p0.x + end.x) / 2, cy = sp * cx1 + cp * cy1 + (p0.y + end.y) / 2;
        auto angle = [](float ux, float uy, float vx, float vy) {
            float a = std::atan2(ux * vy - uy * vx, ux * vx + uy * vy);
            return a;
        };
        float theta = angle(1, 0, (x1 - cx1) / rx, (y1 - cy1) / ry);
        float delta = angle((x1 - cx1) / rx, (y1 - cy1) / ry, (-x1 - cx1) / rx, (-y1 - cy1) / ry);
        if (!sweep && delta > 0) delta -= 2 * IM_PI;
        if (sweep && delta < 0) delta += 2 * IM_PI;
        int steps = ImMax(4, int(std::fabs(delta) / (IM_PI / 12)));
        for (int k = 1; k <= steps; k++) {
            float a = theta + delta * k / steps;
            float ex = rx * std::cos(a), ey = ry * std::sin(a);
            lineTo(k == steps ? end : ImVec2(cp * ex - sp * ey + cx, sp * ex + cp * ey + cy));
        }
    }

    bool command(char cmd, Icon* icon) {
        const bool rel = std::islower(static_cast<unsigned char>(cmd)) != 0;
        const char up = char(std::toupper(static_cast<unsigned char>(cmd)));
        ImVec2 p, c1, c2;
        float v;
        switch (up) {
        case 'M':
            if (!point(rel, &p)) return false;
            finish(icon);
            cur_ = start_ = p;
            break;
        case 'L':
            if (!point(rel, &p)) return false;
            corner();
            lineTo(p);
            corner();
            break;
        case 'H':
            if (!number(&v)) return false;
            corner();
            lineTo(ImVec2(rel ? cur_.x + v : v, cur_.y));
            corner();
            break;
        case 'V':
            if (!number(&v)) return false;
            corner();
            lineTo(ImVec2(cur_.x, rel ? cur_.y + v : v));
            corner();
            break;
        case 'C':
            if (!point(rel, &c1) || !point(rel, &c2) || !point(rel, &p)) return false;
            cubic(c1, c2, p);
            break;
        case 'S': {
            if (!point(rel, &c2) || !point(rel, &p)) return false;
            bool smooth = last_ == 'C' || last_ == 'S';
            ImVec2 r = smooth ? ImVec2(2 * cur_.x - ctrl_.x, 2 * cur_.y - ctrl_.y) : cur_;
            cubic(r, c2, p);
            break;
        }
        case 'Q':
            if (!point(rel, &c1) || !point(rel, &p)) return false;
            quad(c1, p);
            break;
        case 'T': {
            if (!point(rel, &p)) return false;
            bool smooth = last_ == 'Q' || last_ == 'T';
            quad(smooth ? ImVec2(2 * cur_.x - ctrl_.x, 2 * cur_.y - ctrl_.y) : cur_, p);
            break;
        }
        case 'A': {
            float rx, ry, rot;
            bool large, sweep;
            if (!number(&rx) || !number(&ry) || !number(&rot) || !flag(&large) || !flag(&sweep) || !point(rel, &p))
                return false;
            arc(rx, ry, rot, large, sweep, p);
            break;
        }
        case 'Z':
            if (!stroke_.points.empty()) {
                lineTo(start_);
                stroke_.closed = true;
            }
            finish(icon);
            cur_ = start_;
            break;
        default:
            return false;
        }
        last_ = up;
        return true;
    }
};

}  // namespace

Icon ParseIcon(const std::string& svg) {
    Icon icon;
    size_t at = 0;
    while ((at = svg.find(" d=\"", at)) != std::string::npos) {
        at += 4;
        size_t end = svg.find('"', at);
        if (end == std::string::npos) break;
        std::string d = svg.substr(at, end - at);
        PathParser(d).parse(&icon);
        at = end;
    }
    return icon;
}

void DrawIcon(ImDrawList* list, const Icon& icon, ImVec2 pos, float size, ImU32 color) {
    const float k = size / kViewBox;
    const float thickness = ImMax(1.0f, kStroke * k);
    std::vector<ImVec2> pts;
    for (const Icon::Stroke& stroke : icon.strokes) {
        pts.clear();
        for (const ImVec2& p : stroke.points) pts.push_back(ImVec2(pos.x + p.x * k, pos.y + p.y * k));
        if (pts.size() >= 2) {
            list->AddPolyline(pts.data(), int(pts.size()), color, stroke.closed ? ImDrawFlags_Closed : 0, thickness);
        }
        for (const ImVec2& j : stroke.joints) {
            list->AddCircleFilled(ImVec2(pos.x + j.x * k, pos.y + j.y * k), thickness * 0.5f, color, 12);
        }
    }
}

}  // namespace mcui
