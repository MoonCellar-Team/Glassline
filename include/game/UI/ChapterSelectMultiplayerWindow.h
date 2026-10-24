#pragma once

#include "game/UI/ChapterArrowedScroller.h"
#include "game/UI/CheckBox.h"
#include "game/UI/MajorButton.h"
#include "game/UI/TitledWindow.h"

namespace UI {

class ChapterSelectMultiplayerWindow : public TitledWindow {
public:
    ChapterSelectMultiplayerWindow();
    virtual ~ChapterSelectMultiplayerWindow();

    virtual void update(int dt);
    using TitledWindow::render;
    virtual void render(midp::Graphics* g, int originY, int originX);
    virtual bool pointerPressed(int x, int y, int id);
    virtual bool pointerReleased(int x, int y, int id);
    virtual bool pointerDragged(int x, int y, int id);
    virtual void onSelected();

    bool isRaceMode() const;
    int getRivalTimeMillis() const;
    bool getCombatAllowed() const;
#ifdef MEDECOMP_PORT
    void selectLastPlayed();
#endif

    ChapterArrowedScroller m_modeScroller;
    ChapterArrowedScroller m_raceScroller;
    ChapterArrowedScroller m_timedScroller;
    CheckBox m_time2Min;
    CheckBox m_time3Min;
    CheckBox m_time5Min;
    CheckBox m_combat;
    bool m_hasRaceLevels;
    MajorButton m_startButton;
};

}
