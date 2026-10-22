#include "game/GameScreenPointer.h"

static const int HOLD_MS = 500;

bool GameScreenPointer::isPressed() {
    return m_id != -1;
}

float GameScreenPointer::operator==(int id) {
    return m_id == id;
}

void GameScreenPointer::release() {
    m_id = -1;
}

GameScreenPointer::GameScreenPointer()
    : m_id(-1),
      m_pressX(0),
      m_pressY(0),
      m_screenX(0),
      m_screenY(0),
      m_x(0),
      m_y(0),
      m_holdTime(0),
      m_swiped(false),
      m_swipeDir(0) {}

void GameScreenPointer::press(int id, int screenX, int screenY, int x, int y) {
    m_id = id;
    m_pressX = x;
    m_pressY = y;
    m_screenX = screenX;
    m_screenY = screenY;
    m_x = x;
    m_y = y;
    m_holdTime = 0;
    m_swiped = false;
    m_swipeDir = 0;
}

void GameScreenPointer::swipe(int dir, int screenX, int screenY, int x, int y) {
    m_swiped = true;
    m_swipeDir = dir;
    m_screenX = screenX;
    m_screenY = screenY;
    m_x = x;
    m_y = y;
}

bool GameScreenPointer::checkHold(int dt) {
    if (m_id == -1 || m_swipeDir != 0) {
        return false;
    }
    int prev = m_holdTime;
    m_holdTime += dt;
    return prev < HOLD_MS && m_holdTime >= HOLD_MS;
}
