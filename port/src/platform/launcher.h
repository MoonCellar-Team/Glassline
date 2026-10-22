#pragma once

#include <string>

namespace port {
namespace platform {

bool chooseGame(int argc, char** argv);
void syncOnExit();

std::string launcherFont();
bool launcherFonts();
void launcherStyle();
bool launcherRussian();
void panelTouchScroll(float scale);

}
}
