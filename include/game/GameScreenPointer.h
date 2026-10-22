#pragma once

class GameScreenPointer {
public:
    enum SwipeDir {
        SWIPE_LEFT = 1,
        SWIPE_RIGHT = 2,
        SWIPE_UP = 4,
        SWIPE_DOWN = 8,
    };

    GameScreenPointer();

    bool isPressed();
    float operator==(int id);
    void release();
    void press(int id, int screenX, int screenY, int x, int y);
    void swipe(int dir, int screenX, int screenY, int x, int y);
    bool checkHold(int dt);

    int m_id;
    int m_pressX;
    int m_pressY;
    int m_screenX;
    int m_screenY;
    int m_x;
    int m_y;
    int m_holdTime;
    bool m_swiped;
    int m_swipeDir;
};
