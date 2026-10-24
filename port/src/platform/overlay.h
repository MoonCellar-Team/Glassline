#pragma once

#include <string>

#include "branding.h"

namespace port {
namespace platform {

void notify(const std::string& title, const std::string& text, const std::string& iconSvg = std::string(),
            float seconds = 5.0f);

void overlayDraw(int w, int h);
void overlayCover(int w, int h);
bool helperRun();

bool splashSetting();
void setSplashSetting(bool on);
void splashBegin();
bool splashActive();
bool splashSkip();

}
}
