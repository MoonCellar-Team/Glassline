#pragma once

class GhostAnimation {
public:
    static const int FILE_VERSION = 3;

    static char GHOST_FILENAME[];

    static const char* getCurrentLevelGhostFilename();
};
