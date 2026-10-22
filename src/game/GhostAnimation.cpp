#include "game/GhostAnimation.h"

#include "game/AppEngine.h"
#include "game/LevelData.h"

#ifdef MEDECOMP_PORT
extern "C" const char* medecomp_player_ghost_filename(void);
#endif

char GhostAnimation::GHOST_FILENAME[] = "GhostS--";

const char* GhostAnimation::getCurrentLevelGhostFilename() {
#ifdef MEDECOMP_PORT
    if (const char* name = medecomp_player_ghost_filename()) return name;
#endif
    int level = AppEngine::getLevelData()->getCurrentLevelIndex();
    GHOST_FILENAME[6] = (char)('0' + level / 10);
    GHOST_FILENAME[7] = (char)('0' + level % 10);
    return GHOST_FILENAME;
}
