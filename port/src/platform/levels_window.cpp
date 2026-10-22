#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../ea/cloud.h"
#include "../ea/json.h"
#include "../ea/levels.h"
#include "game/AppEngine.h"
#include "game/LevelData.h"
#include "game/SceneMenu.h"
#include "game/TextManager.h"
#include "game/UI/Button.h"
#include "game/UI/LeftArrowButton.h"
#include "game/UI/MajorButton.h"
#include "game/UI/RightArrowButton.h"
#include "game/UI/TitledWindow.h"
#include "game/UI/WindowStore.h"
#include "levels.h"
#include "port/cf.h"
#include "port/files.h"
#include "port/online.h"
#include "port/platform.h"
#include "runtime/midp/Graphics.h"
#include "runtime/midp/String.h"

extern "C" int medecomp_player_level_base(void);
extern "C" void medecomp_player_level_apply(int base);
extern "C" const char* medecomp_line(const void* owner, int limit, int* key);
extern "C" void medecomp_line_close(const void* owner);

namespace {

enum Text {
    TEXT_LEVELS = 0x99e, TEXT_DELETE = 0x9a1, TEXT_EMPTY = 0x9a2,
    TEXT_CONFIRM = 0x9a3, TEXT_BROKEN = 0x9a4, TEXT_ONLINE = 0x9a5, TEXT_MINE = 0x9a6, TEXT_UPLOAD = 0x9a7,
    TEXT_RATE = 0x9a8, TEXT_REPORT = 0x9a9, TEXT_SORT = 0x9aa, TEXT_SEARCH = 0x9ab, TEXT_ONLINE_NEW = 0x9ac,
    TEXT_ONLINE_BEST = 0x9ad, TEXT_LOADING = 0x9ae, TEXT_OFFLINE = 0x9af, TEXT_UPLOADED = 0x9b0,
    TEXT_SIGN_IN = 0x9b1, TEXT_SCRATCH_ONLY = 0x9b2, TEXT_RATED = 0x9b3, TEXT_REPORT_CONFIRM = 0x9b4,
    TEXT_REPORTED = 0x9b5, TEXT_NONE_FOUND = 0x9b6, TEXT_TYPE_SEARCH = 0x9b7, TEXT_BEST = 0x9b8,
    TEXT_RATING = 0x9b9, TEXT_UNPUBLISH = 0x9bf, TEXT_DELETE_ONLINE_CONFIRM = 0x9c0,
    TEXT_UNPUBLISH_CONFIRM = 0x9c1, TEXT_REMOVED = 0x9c2, TEXT_COLUMN_LEVEL = 0x9c3, TEXT_COLUMN_AUTHOR = 0x9c4,
    TEXT_COLUMN_SAVED = 0x9c5, TEXT_COLUMN_MODE = 0x9c7, TEXT_FILTER_ALL = 0x9c8, TEXT_OTHER_ACCOUNT = 0x9dc,
    TEXT_PLAY = 0x906,
    TEXT_RACE = 0x941,
    TEXT_TIMED = 0x942,
};

const int FONT_ROW = 2;
const int FONT_HEAD = 3;
const int FONT_INFO = 3;
const int ANCHOR_CENTER_TOP = 0x0a;
const int ANCHOR_LEFT_VCENTER = 0x11;
const int ANCHOR_RIGHT_VCENTER = 0x14;

const int BOX_X = 110, BOX_Y = 200, BOX_W = 804, BOX_H = 330;
const int HEAD_Y = BOX_Y + 22;
const int WHERE_ABOVE = 16;
const int FIRST_ROW = BOX_Y + 42;
const int ROW_H = 35, ROWS = 8;
const int BUTTONS_Y = 548, BUTTONS_GAP_Y = 56, BUTTONS_GAP_X = 24;
const int STATUS_Y = 660;
const int ARROW_INSET = 36, COLUMN_GAP = 14;
const int SELECTED_ARGB = int(0xFFF8D3D3u);

const int RATE_DELAY_MS = 1500;
const int SEARCH_LIMIT = 40;
const int KEY_RETURN = 13, KEY_ESCAPE = 27;

struct Mine {
    std::string id, name, author, board, serverId;
    bool bags = false;
};

enum Filter { FILTER_ALL, FILTER_RACE, FILTER_BAGS };

std::string levelsDir() { return port::files::dataDir() + "/levels"; }

std::vector<Mine> readMine(Filter filter) {
    std::vector<Mine> out;
    for (const std::string& id : port::files::list(levelsDir())) {
        std::vector<unsigned char> json;
        port::online::Json info;
        if (!port::files::read(levelsDir() + "/" + id + "/level.json", &json) ||
            !port::online::parseJson(std::string(json.begin(), json.end()), &info))
            continue;
        bool bags = info["mode"].str() == "bags";
        if ((filter == FILTER_RACE && bags) || (filter == FILTER_BAGS && !bags)) continue;
        out.push_back({id, info["name"].str(id), info["author"].str(), info["server"]["board"].str(),
                       port::online::uploadedId(id), bags});
    }
    std::sort(out.begin(), out.end(), [](const Mine& a, const Mine& b) { return a.id < b.id; });
    return out;
}

midp::String* text(const std::string& utf8) {
    return new midp::String(static_cast<const __CFString*>(port::cf::makeString(utf8)));
}

TextManager* texts() { return AppEngine::getCanvas()->getTextManager(); }

std::string word(int id) {
    TextManager* t = texts();
    midp::String* s = t->getLangString(id, t->getCurrentLanguage());
    return s ? port::cf::utf8(s->getCFString()) : std::string();
}

int widthOf(const std::string& s, int font) {
    midp::ReferenceCountedPointer<midp::String> str(text(s));
    return texts()->getStringWidth((midp::String*)str, font);
}

std::string fit(const std::string& s, int font, int max) {
    if (widthOf(s, font) <= max) return s;
    std::string cut = s;
    while (!cut.empty()) {
        while (!cut.empty() && (static_cast<unsigned char>(cut.back()) & 0xC0) == 0x80) cut.pop_back();
        if (!cut.empty()) cut.pop_back();
        if (widthOf(cut + "…", font) <= max) return cut + "…";
    }
    return "…";
}

std::vector<std::string> wrap(const std::string& s, int font, int max) {
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < s.size()) {
        size_t len = 1;
        while (i + len < s.size() && (static_cast<unsigned char>(s[i + len]) & 0xC0) == 0x80) len++;
        std::string next = line + s.substr(i, len);
        if (!line.empty() && widthOf(next, font) > max) {
            size_t space = line.rfind(' ');
            if (space != std::string::npos && s[i] != ' ') {
                lines.push_back(line.substr(0, space));
                line = line.substr(space + 1) + s.substr(i, len);
            } else {
                lines.push_back(line);
                line = s[i] == ' ' ? std::string() : s.substr(i, len);
            }
        } else {
            line = next;
        }
        i += len;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

std::string timeText(int ms) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d:%02d.%02d", ms / 60000, ms / 1000 % 60, ms / 10 % 100);
    return buf;
}

}

namespace port {
namespace platform {

class LevelRow : public UI::Button {
public:
    void render(midp::Graphics* g, int originY, int originX) override;
};

void LevelRow::render(midp::Graphics*, int, int) {}

class PlayerLevelsWindow : public UI::TitledWindow {
public:
    PlayerLevelsWindow() : TitledWindow(TEXT_LEVELS, TEXT_MINE) {
        m_marginX = m_marginY = 0;
        adjustClientArea();
        m_background.setPosition(BOX_X, BOX_Y);
        m_background.setDimensions(BOX_W, BOX_H);
        m_left->setPosition(BOX_X + ARROW_INSET, BOX_Y + (BOX_H - m_left->getHeight()) / 2);
        m_right->setPosition(BOX_X + BOX_W - ARROW_INSET - m_right->getWidth(), BOX_Y + (BOX_H - m_right->getHeight()) / 2);
        for (int i = 0; i < ROWS; i++) {
            m_rows[i] = new LevelRow();
            m_rows[i]->setPosition(contentLeft(), FIRST_ROW + i * ROW_H);
            m_rows[i]->setDimensions(contentRight() - contentLeft(), ROW_H);
        }
        m_mine = readMine(m_filter);
        showMode();
        syncWithServer();
        bestTimes();
    }

    ~PlayerLevelsWindow() override {
        if (m_thread.joinable()) m_thread.join();
        medecomp_line_close(this);
        for (UI::Button* b : all()) {
            removeElement(b);
            delete b;
        }
    }

    void update(int dt) override {
        TitledWindow::update(dt);
        std::vector<std::function<void()>> applies;
        {
            std::lock_guard<std::mutex> lock(m_mu);
            applies.swap(m_applies);
        }
        for (auto& apply : applies) apply();
        if (!m_busy && !m_queue.empty()) {
            std::function<void(const Post&)> next = m_queue.front();
            m_queue.pop_front();
            run(next);
        }
        if (m_searching) typeSearch();
        if (m_rateMs > 0 && (m_rateMs -= dt) <= 0) sendRating();
        const void* focus = port::platform::menuFocusTarget();
        if (focus != m_lastFocus) {
            m_lastFocus = focus;
            for (int i = 0; i < rowCount(); i++)
                if (focus == m_rows[i]) select(i), m_chosen = true;
        }
        for (int i = 0; i < rowCount(); i++)
            if (fired(m_rows[i])) {
                if (m_pressedSelected == i && m_chosen) playSelected();
                else select(i), m_chosen = true;
            }
        bool any = rowCount() > 0;
        for (UI::Button* b : {m_play, m_delete, m_rate, m_report, m_unpublish, m_upload, m_sort})
            b->setEnabled(disabledReason(b) < 0);
        int total = m_online ? m_total : int(m_mine.size());
        m_left->setEnabled(m_first > 0);
        m_right->setEnabled(m_first + ROWS < total);
        if (fired(m_left)) page(-1);
        if (fired(m_right)) page(1);
        if (fired(m_toOnline)) setMode(true);
        if (fired(m_toMine)) setMode(false);
        if (fired(m_delete) && any) {
            if (m_online) {
                if (m_note == TEXT_DELETE_ONLINE_CONFIRM) removeOnline(m_net[size_t(m_sel)].id);
                else m_note = TEXT_DELETE_ONLINE_CONFIRM;
            } else if (m_note == TEXT_CONFIRM) {
                removeMine(), m_note = -1;
            } else {
                m_note = TEXT_CONFIRM;
            }
        }
        if (fired(m_unpublish) && any && !m_online) {
            if (m_note == TEXT_UNPUBLISH_CONFIRM) removeOnline(m_mine[size_t(m_first + m_sel)].serverId);
            else m_note = TEXT_UNPUBLISH_CONFIRM;
        }
        if (fired(m_upload) && any) upload();
        if (fired(m_play) && any) playSelected();
        if (fired(m_rate) && any) {
            m_stars = m_stars % 5 + 1;
            m_rateMs = RATE_DELAY_MS;
            m_note = -1;
        }
        if (fired(m_report) && any) {
            if (m_note == TEXT_REPORT_CONFIRM) report();
            else m_note = TEXT_REPORT_CONFIRM;
        }
        if (fired(m_sort)) {
            m_best = !m_best;
            setTitles(TEXT_LEVELS, m_best ? TEXT_ONLINE_BEST : TEXT_ONLINE_NEW);
            fetch(0);
        }
        if (fired(m_search)) m_searching = true, m_note = -1;
        for (UI::Button* b : {m_filterAll, m_filterRace, m_filterBags})
            if (fired(b)) setFilter(Filter((int(m_filter) + 1) % 3));
    }

    bool pointerPressed(int x, int y, int id) override {
        TitledWindow::pointerPressed(x, y, id);
        m_pressedSelected = -1;
        for (int i = 0; i < rowCount(); i++)
            if (m_rows[i]->contains(x, y) && m_sel == i) m_pressedSelected = i;
        for (UI::Button* b : shown()) {
            if (!b->contains(x, y)) continue;
            int why = disabledReason(b);
            if (why >= 0) m_note = why;
            else b->pointerPressed(x, y, id);
        }
        return true;
    }
    bool pointerReleased(int x, int y, int id) override {
        TitledWindow::pointerReleased(x, y, id);
        for (UI::Button* b : shown()) {
            if (b->contains(x, y) && b->isPressed()) b->pointerReleased(x, y, id);
            else if (b->isPressed()) b->unpress();
        }
        return true;
    }

    void render(midp::Graphics* g, int originY, int originX) override {
        TitledWindow::render(g, originY, originX);
        for (UI::Button* b : shown()) b->render(g, originY, originX);
        TextManager* t = texts();
        auto draw = [&](const std::string& s, int font, int x, int y, int anchor) {
            midp::ReferenceCountedPointer<midp::String> str(text(s));
            t->drawString(g, (midp::String*)str, font, x + originX, y + originY, anchor);
        };
        int left = contentLeft(), width = contentRight() - left;
        if (rowCount() == 0) {
            std::string s = word(emptyText());
            std::vector<std::string> lines = wrap(s, FONT_INFO, width);
            int lineH = t->getLineHeight(FONT_INFO);
            int y = BOX_Y + (BOX_H - int(lines.size()) * lineH) / 2;
            for (const std::string& l : lines) draw(l, FONT_INFO, left + width / 2, y, ANCHOR_CENTER_TOP), y += lineH;
        } else {
            std::vector<std::pair<int, int>> cols = columns(left, width);
            std::vector<int> heads = m_online ? std::vector<int>{TEXT_COLUMN_LEVEL, TEXT_COLUMN_AUTHOR, TEXT_COLUMN_MODE,
                                                                 TEXT_RATING, TEXT_BEST, TEXT_COLUMN_SAVED}
                                              : std::vector<int>{TEXT_COLUMN_LEVEL, TEXT_COLUMN_AUTHOR, TEXT_COLUMN_MODE,
                                                                 TEXT_ONLINE, TEXT_BEST};
            for (size_t c = 0; c < cols.size(); c++)
                draw(fit(word(heads[c]), FONT_HEAD, cols[c].second), FONT_HEAD, cols[c].first, HEAD_Y,
                     ANCHOR_LEFT_VCENTER);
            for (int i = 0; i < rowCount(); i++) {
                int y = FIRST_ROW + i * ROW_H;
                if (i == m_sel) {
                    g->setColor(SELECTED_ARGB);
                    g->fillRect(left - 6 + originX, y + originY, width + 12, ROW_H);
                }
                std::vector<std::string> cells = rowCells(i);
                for (size_t c = 0; c < cols.size() && c < cells.size(); c++)
                    draw(fit(cells[c], FONT_ROW, cols[c].second), FONT_ROW, cols[c].first, y + ROW_H / 2,
                         ANCHOR_LEFT_VCENTER);
            }
            int total = m_online ? m_total : int(m_mine.size());
            std::string where = std::to_string(m_first + 1) + "–" + std::to_string(m_first + rowCount()) + " / " +
                                std::to_string(total);
            draw(where, FONT_HEAD, BOX_X + BOX_W - COLUMN_GAP, BOX_Y - WHERE_ABOVE, ANCHOR_RIGHT_VCENTER);
        }
        int cx = 512;
        auto note = [&](const std::string& s) {
            std::vector<std::string> lines = wrap(s, FONT_INFO, BOX_W);
            int y = STATUS_Y;
            for (const std::string& l : lines) draw(l, FONT_INFO, cx, y, ANCHOR_CENTER_TOP), y += t->getLineHeight(FONT_INFO);
        };
        if (m_busy && !(rowCount() == 0 && m_pageWaiting)) note(word(TEXT_LOADING));
        else if (m_searching && m_query.empty()) note(word(TEXT_TYPE_SEARCH));
        else if (m_searching) note(word(TEXT_SEARCH) + ": " + m_query + "_");
        else if (m_rateMs > 0) note(word(TEXT_RATE) + ": " + std::to_string(m_stars) + " / 5");
        else if (m_note >= 0) note(word(m_note));
    }

private:
    UI::Button* m_left = new UI::LeftArrowButton();
    UI::Button* m_right = new UI::RightArrowButton();
    UI::MajorButton* m_play = new UI::MajorButton(TEXT_PLAY);
    UI::MajorButton* m_delete = new UI::MajorButton(TEXT_DELETE);
    UI::MajorButton* m_upload = new UI::MajorButton(TEXT_UPLOAD);
    UI::MajorButton* m_unpublish = new UI::MajorButton(TEXT_UNPUBLISH);
    UI::MajorButton* m_toOnline = new UI::MajorButton(TEXT_ONLINE);
    UI::MajorButton* m_rate = new UI::MajorButton(TEXT_RATE);
    UI::MajorButton* m_report = new UI::MajorButton(TEXT_REPORT);
    UI::MajorButton* m_sort = new UI::MajorButton(TEXT_SORT);
    UI::MajorButton* m_search = new UI::MajorButton(TEXT_SEARCH);
    UI::MajorButton* m_toMine = new UI::MajorButton(TEXT_MINE);
    UI::MajorButton* m_filterAll = new UI::MajorButton(TEXT_FILTER_ALL);
    UI::MajorButton* m_filterRace = new UI::MajorButton(TEXT_RACE);
    UI::MajorButton* m_filterBags = new UI::MajorButton(TEXT_TIMED);
    LevelRow* m_rows[ROWS] = {};

    bool m_online = false, m_best = false, m_searching = false, m_canUpload = false;
    bool m_otherAccount = false;
    Filter m_filter = FILTER_ALL;
    std::vector<Mine> m_mine;
    std::vector<port::online::NetLevel> m_net;
    std::map<std::string, std::pair<std::string, int>> m_bests;
    int m_first = 0;
    int m_sel = 0, m_total = 0, m_note = -1, m_stars = 0, m_rateMs = 0, m_pressedSelected = -1;
    std::atomic<int> m_gen{0};
    bool m_pageWaiting = false, m_pageFailed = false;
    const void* m_lastFocus = nullptr;
    bool m_chosen = false;
    std::string m_query;

    std::thread m_thread;
    std::atomic<bool> m_busy{false};
    std::mutex m_mu;
    std::vector<std::function<void()>> m_applies;

    int contentLeft() const { return m_left->getX() + m_left->getWidth() + COLUMN_GAP; }
    int contentRight() const { return m_right->getX() - COLUMN_GAP; }
    std::vector<std::pair<int, int>> columns(int left, int width) const {
        std::vector<int> parts = m_online ? std::vector<int>{22, 16, 22, 14, 14, 12} : std::vector<int>{26, 18, 24, 14, 18};
        std::vector<std::pair<int, int>> out;
        int x = left;
        for (int p : parts) {
            int w = width * p / 100;
            out.push_back({x, w - COLUMN_GAP / 2});
            x += w;
        }
        return out;
    }
    std::vector<std::string> rowCells(int i) const {
        auto best = [&](const std::string& board) {
            auto it = m_bests.find(board);
            return it == m_bests.end() ? std::string() : timeText(it->second.second);
        };
        if (m_online) {
            const port::online::NetLevel& l = m_net[size_t(i)];
            char rating[32];
            snprintf(rating, sizeof rating, l.ratings ? "%.1f (%d)" : "—", l.rating, l.ratings);
            bool saved = port::files::stat(levelsDir() + "/net-" + l.id + "/level.json").exists;
            bool bags = l.mode == "bags";
            return {l.name, l.author, word(bags ? TEXT_TIMED : TEXT_RACE), rating, bags ? std::string() : best(l.board),
                    saved ? "•" : ""};
        }
        const Mine& l = m_mine[size_t(m_first + i)];
        bool online = !l.serverId.empty() || l.id.compare(0, 4, "net-") == 0;
        return {l.name, l.author.empty() ? l.id : l.author, word(l.bags ? TEXT_TIMED : TEXT_RACE), online ? "•" : "",
                l.bags ? port::platform::bestBags(l.id) : best(l.board)};
    }

    int rowCount() const {
        if (m_online) return int(m_net.size());
        return std::max(0, std::min(ROWS, int(m_mine.size()) - m_first));
    }
    std::vector<UI::Button*> all() {
        std::vector<UI::Button*> out = {m_left,     m_right,  m_play,   m_delete, m_upload,    m_unpublish,
                                        m_toOnline, m_rate,   m_report, m_sort,   m_search,    m_toMine,
                                        m_filterAll, m_filterRace, m_filterBags};
        for (LevelRow* r : m_rows) out.push_back(r);
        return out;
    }
    bool ownOnline(const port::online::NetLevel& l) const {
        std::string user = port::online::userName();
        if (!user.empty() && l.author == user) return true;
        for (const Mine& m : m_mine)
            if (m.serverId == l.id) return true;
        return false;
    }
    std::vector<UI::Button*> rowOne() {
        if (m_online) {
            std::vector<UI::Button*> row = {m_play, m_rate, m_report};
            if (rowCount() > 0 && ownOnline(m_net[size_t(m_sel)])) row.push_back(m_delete);
            row.push_back(m_sort);
            return row;
        }
        std::vector<UI::Button*> row = {m_play, m_delete, m_upload};
        if (rowCount() > 0 && !m_mine[size_t(m_first + m_sel)].serverId.empty()) row.push_back(m_unpublish);
        return row;
    }
    UI::Button* filterButton() const {
        return m_filter == FILTER_RACE ? m_filterRace : m_filter == FILTER_BAGS ? m_filterBags : m_filterAll;
    }
    std::vector<UI::Button*> rowTwo() {
        if (m_online) return {m_toMine, m_search, filterButton()};
        return {m_toOnline, filterButton()};
    }
    void setFilter(Filter filter) {
        m_filter = filter;
        m_note = -1;
        m_first = m_sel = 0;
        m_chosen = false;
        if (m_online) {
            layout();
            fetch(0);
        } else {
            m_mine = readMine(m_filter);
            bestTimes();
        }
    }
    int emptyText() const {
        if (!m_online) return TEXT_EMPTY;
        return m_pageWaiting ? TEXT_LOADING : m_pageFailed ? TEXT_OFFLINE : TEXT_NONE_FOUND;
    }
    std::string filterMode() const { return m_filter == FILTER_RACE ? "race" : m_filter == FILTER_BAGS ? "bags" : ""; }
    std::vector<UI::Button*> shown() {
        std::vector<UI::Button*> out = {m_left, m_right};
        for (int i = 0; i < rowCount(); i++) out.push_back(m_rows[i]);
        for (UI::Button* b : rowOne()) out.push_back(b);
        for (UI::Button* b : rowTwo()) out.push_back(b);
        return out;
    }
    int disabledReason(UI::Button* b) const {
        bool any = rowCount() > 0;
        bool signedIn = !port::online::token().empty();
        if (!any && (b == m_play || b == m_delete || b == m_rate || b == m_report || b == m_upload ||
                     b == m_unpublish || b == m_sort))
            return emptyText();
        if (!signedIn && (b == m_upload || b == m_rate || b == m_report || b == m_unpublish || (m_online && b == m_delete)))
            return TEXT_SIGN_IN;
        if ((b == m_upload || b == m_unpublish) && m_otherAccount) return TEXT_OTHER_ACCOUNT;
        if (b == m_upload && !m_canUpload) return TEXT_SCRATCH_ONLY;
        return -1;
    }
    static bool fired(UI::Button* b) {
        if (!b->isActivated()) return false;
        b->reset();
        return true;
    }

    void layout() {
        for (UI::Button* b : all()) removeElement(b);
        int w = m_play->getWidth();
        std::vector<UI::Button*> rows[2] = {rowOne(), rowTwo()};
        for (int r = 0; r < 2; r++) {
            int x = (1024 - (int(rows[r].size()) * w + (int(rows[r].size()) - 1) * BUTTONS_GAP_X)) / 2;
            for (size_t i = 0; i < rows[r].size(); i++)
                rows[r][i]->setPosition(x + int(i) * (w + BUTTONS_GAP_X), BUTTONS_Y + r * BUTTONS_GAP_Y);
        }
        for (UI::Button* b : shown()) addElement(b);
    }
    void showMode() {
        layout();
        setTitles(TEXT_LEVELS, m_online ? (m_best ? TEXT_ONLINE_BEST : TEXT_ONLINE_NEW) : TEXT_MINE);
    }
    void setMode(bool online) {
        m_online = online;
        m_first = m_sel = 0;
        m_chosen = false;
        m_note = -1;
        m_searching = false;
        m_gen++;
        medecomp_line_close(this);
        m_mine = readMine(m_filter);
        if (online) {
            m_net.clear();
            showMode();
            fetch(0);
        } else {
            showMode();
            bestTimes();
        }
    }
    void select(int i) {
        if (i < 0 || i >= rowCount()) return;
        bool changed = i != m_sel;
        m_sel = i;
        if (changed) {
            m_note = -1;
            m_stars = 0;
            m_rateMs = 0;
        }
        m_canUpload = !m_online && port::online::uploadable(m_mine[size_t(m_first + m_sel)].id);
        m_otherAccount = !m_online && port::online::onlineState(m_mine[size_t(m_first + m_sel)].id).state ==
                                          port::online::Online::OtherAccount;
        layout();
    }
    void page(int d) {
        int total = m_online ? m_total : int(m_mine.size());
        int first = m_first + d * ROWS;
        if (first < 0 || first >= total) return;
        m_note = -1;
        m_chosen = false;
        if (m_online) {
            fetch(first);
        } else {
            m_first = first;
            m_sel = 0;
            select(0);
            bestTimes();
        }
    }

    using Post = std::function<void(std::function<void()>)>;
    std::deque<std::function<void(const Post&)>> m_queue;
    void run(std::function<void(const Post&)> work) {
        if (m_busy) {
            m_queue.push_back(work);
            return;
        }
        if (m_thread.joinable()) m_thread.join();
        m_busy = true;
        m_thread = std::thread([this, work] {
            work([this](std::function<void()> apply) {
                std::lock_guard<std::mutex> lock(m_mu);
                m_applies.push_back(apply);
            });
            m_busy = false;
        });
    }
    static void fetchBests(std::vector<std::string> boards, const Post& post, std::function<void(std::string, std::pair<std::string, int>)> put,
                           std::function<bool()> stale) {
        for (const std::string& b : boards) {
            if (stale()) return;
            if (b.empty()) continue;
            std::vector<std::pair<std::string, int>> rows;
            std::string error;
            if (port::online::levelLeaderboard(b, 1, &rows, &error) && !rows.empty())
                post([put, b, row = rows[0]] { put(b, row); });
        }
    }
    void fetch(int first) {
        std::string sort = m_best ? "best" : "new", q = m_query, mode = filterMode();
        int gen = ++m_gen;
        m_pageWaiting = true;
        m_pageFailed = false;
        run([this, sort, q, mode, first, gen](const Post& post) {
            if (gen != m_gen) return;
            std::vector<port::online::NetLevel> list;
            int total = 0;
            std::string error;
            bool ok = port::online::listLevels(sort, q, first, ROWS, &list, &total, &error, mode);
            post([this, ok, list, total, first, gen] {
                if (gen != m_gen) return;
                m_net = list;
                m_total = total;
                m_first = first;
                m_sel = 0;
                m_chosen = false;
                m_pageWaiting = false;
                m_pageFailed = !ok;
                m_note = ok || list.empty() ? -1 : TEXT_OFFLINE;
                layout();
            });
            std::vector<std::string> boards;
            for (const auto& l : list) boards.push_back(l.board);
            fetchBests(
                boards, post,
                [this, gen](std::string b, std::pair<std::string, int> row) {
                    if (gen == m_gen) m_bests[b] = row;
                },
                [this, gen] { return gen != m_gen; });
        });
    }
    void syncWithServer() {
        if (port::online::token().empty()) return;
        run([this](const Post& post) {
            int changed = 0;
            std::string error;
            if (!port::online::syncUploaded(&changed, &error))
                fprintf(stderr, "medecomp: сверка своих уровней с сервером: %s\n", error.c_str());
            if (changed == 0) return;
            post([this] {
                port::online::cloud::syncLevels();
                if (m_online) return;
                m_mine = readMine(m_filter);
                bestTimes();
            });
        });
    }
    void bestTimes() {
        std::vector<std::string> boards;
        for (int i = 0; i < rowCount(); i++) boards.push_back(m_mine[size_t(m_first + i)].board);
        int gen = m_gen;
        run([this, boards, gen](const Post& post) {
            fetchBests(
                boards, post,
                [this, gen](std::string b, std::pair<std::string, int> row) {
                    if (gen == m_gen) m_bests[b] = row;
                },
                [this, gen] { return gen != m_gen; });
        });
        if (rowCount() > 0) select(std::min(m_sel, rowCount() - 1));
        else layout();
    }
    void typeSearch() {
        int key = 0;
        m_query = medecomp_line(this, SEARCH_LIMIT, &key);
        if (key == KEY_RETURN || key == KEY_ESCAPE) {
            m_searching = false;
            medecomp_line_close(this);
            if (key == KEY_RETURN) fetch(0);
        }
    }
    void upload() {
        if (port::online::token().empty()) {
            m_note = TEXT_SIGN_IN;
            return;
        }
        std::string id = m_mine[size_t(m_first + m_sel)].id;
        if (!port::online::uploadable(id)) {
            m_note = TEXT_SCRATCH_ONLY;
            return;
        }
        run([id, this](const Post& post) {
            std::string error;
            bool ok = port::online::uploadLevel(id, &error);
            post([this, ok] {
                m_note = ok ? TEXT_UPLOADED : TEXT_OFFLINE;
                m_mine = readMine(m_filter);
                select(m_sel);
                if (ok) port::online::cloud::syncLevels();
            });
        });
    }
    void removeOnline(const std::string& serverId) {
        if (port::online::token().empty()) {
            m_note = TEXT_SIGN_IN;
            return;
        }
        bool online = m_online;
        std::string sort = m_best ? "best" : "new", q = m_query, mode = filterMode();
        int first = m_first;
        run([this, serverId, online, sort, q, mode, first](const Post& post) {
            std::string error;
            bool ok = port::online::deleteLevel(serverId, &error);
            post([this, ok, online] {
                m_note = ok ? TEXT_REMOVED : TEXT_OFFLINE;
                m_mine = readMine(m_filter);
                if (!online) select(m_sel);
                if (ok) port::online::cloud::syncLevels();
            });
            if (ok && online) {
                std::vector<port::online::NetLevel> list;
                int total = 0;
                std::string listError;
                if (port::online::listLevels(sort, q, first, ROWS, &list, &total, &listError, mode))
                    post([this, list, total, first] {
                        m_net = list;
                        m_total = total;
                        m_first = list.empty() && first >= ROWS ? first - ROWS : first;
                        m_sel = std::max(0, std::min(m_sel, int(list.size()) - 1));
                        layout();
                    });
            }
        });
    }
    void sendRating() {
        m_rateMs = 0;
        if (port::online::token().empty()) {
            m_note = TEXT_SIGN_IN;
            return;
        }
        std::string id = m_net[size_t(m_sel)].id;
        int stars = m_stars;
        m_stars = 0;
        run([id, stars, this](const Post& post) {
            std::string error;
            bool ok = port::online::rateLevel(id, stars, &error);
            post([this, ok] { m_note = ok ? TEXT_RATED : TEXT_OFFLINE; });
        });
    }
    void report() {
        if (port::online::token().empty()) {
            m_note = TEXT_SIGN_IN;
            return;
        }
        std::string id = m_net[size_t(m_sel)].id;
        run([id, this](const Post& post) {
            std::string error;
            bool ok = port::online::reportLevel(id, "Reported from the game", &error);
            post([this, ok] { m_note = ok ? TEXT_REPORTED : TEXT_OFFLINE; });
        });
    }
    void playSelected() {
        if (rowCount() == 0) return;
        if (m_online) playNet();
        else play(m_mine[size_t(m_first + m_sel)].id);
    }
    void playNet() {
        port::online::NetLevel level = m_net[size_t(m_sel)];
        run([level, this](const Post& post) {
            std::string localId, error;
            bool ok = port::online::downloadLevel(level, &localId, &error);
            post([this, ok, localId] {
                if (ok) play(localId);
                else m_note = TEXT_OFFLINE;
            });
        });
    }
    void removeMine() {
        std::string dir = levelsDir() + "/" + m_mine[size_t(m_first + m_sel)].id;
        for (const std::string& name : port::files::list(dir)) ::remove((dir + "/" + name).c_str());
        if (rmdir(dir.c_str()) != 0) fprintf(stderr, "medecomp: не удалить %s\n", dir.c_str());
        m_mine = readMine(m_filter);
        if (m_first >= int(m_mine.size())) m_first = std::max(0, m_first - ROWS);
        m_sel = std::max(0, std::min(m_sel, rowCount() - 1));
        bestTimes();
    }
    void play(const std::string& id) {
        std::string error;
        if (!preparePlayerLevel(id, &error)) {
            fprintf(stderr, "medecomp: уровень %s: %s\n", id.c_str(), error.c_str());
            m_note = TEXT_BROKEN;
            return;
        }
        int base = medecomp_player_level_base();
        if (base < 0) {
            m_note = TEXT_BROKEN;
            return;
        }
        close(UI::RESULT_NONE);
        medecomp_player_level_apply(base);
        AppEngine::getCanvas()->getSceneMenu()->stateTransitionFade(SceneMenu::STATE_TRANSITION_TO_GAME);
    }
};

}
}

static bool s_wanted;
extern "C" void medecomp_want_player_levels(void) { s_wanted = true; }
extern "C" bool medecomp_take_player_levels(void) {
    if (!s_wanted) return false;
    s_wanted = false;
    UI::Window* w = new port::platform::PlayerLevelsWindow();
    w->setDeleteOnClosed(true);
    AppEngine::getCanvas()->getWindowStore().pushWindow(w);
    return true;
}
