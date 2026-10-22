# Glassline level editor

*Русская версия — [README.ru.md](README.ru.md).*

The level editor builds side-view levels for Glassline out of the game's own parts: blocks and ramps you run
on, scenery pieces from a chapter's palette, bags, checkpoints, zip lines, swing bars and music. A level
stores only its layout. Models, textures and sounds come from each player's own copy of the game, so a
shared level weighs a few kilobytes and carries nothing from the game itself.

- [Interface](interface.md) — every panel, button and shortcut: the tool rail, the field, the level card,
  the inspector, the area bar, the top view, the palette, mouse, keyboard and touch.
- [Objects](objects.md) — how to add each kind of object, what its settings mean, and step-by-step
  examples: a gap to jump, a building, a billboard you run on, a wall run, a swing bar, a turning
  checkpoint dish and more.
- [Levels](levels.md) — creating, saving and testing a level, the Levels tab of the launcher (upload,
  update, take from the server, export, import, delete) and playing levels alone, online and together.

## Opening the editor

- **PC:** in the launcher open the **Levels** tab. **Create level** opens the editor with the new-level
  form; **Edit** next to a level opens that level. The editor runs in its own window, the launcher stays
  open.
- **Android:** the same buttons on the **Levels** tab; the editor opens in place of the launcher and
  comes back to it when you quit. Everything works with fingers (see [touch](interface.md#touch)).

The editor needs your copy of the game chosen on the **Game** tab: the palette, previews and test runs
come from it.

## Quick start

1. **Create level** → type an id (latin letters, digits, `-` and `_`), choose a base chapter, press
   **Create from scratch**. The new level has a floor, a start on the left and a finish on the right.
2. Pick the **Block** tool on the left rail (or press `B`) and click the field to place blocks; drag a
   block to move it, drag its edge or corner to resize it.
3. Pick the **Palette piece** tool, choose a piece in the palette and click the field: this is what the
   level looks like in the game. Blocks are what you run on; pieces are what you see.
4. Select a block and press **Cover with a piece…** to dress it with copies of a roof or a ledge, or give
   it a **surface** — a building's facade over the whole block.
5. Press **Test** (`F5`): the level is saved and starts in the game. Close the game to return.
6. Back in the launcher press **Upload** next to the level to share it (a MoonCellar account is needed).

## Sizes to build with

The field is measured in the game's units. Faith's moves, as the game computes them:

| What | Size |
|---|---|
| Running speed | 7 units per second |
| Height of a jump | 1.3 |
| Running jump over flat ground | about 7 (comfortable gap — 5) |
| Faith's height | 1.8 (room to run under a ceiling — 2, to jump — 3.1) |
| Step she takes without a jump | 1 |
| Ledge she climbs from a jump | up to about 2.3 (2 is safe) |
| Deadly fall | about 16 down |
| Bag she reaches in a jump | up to about 3.6 above the floor |
| Swing bar reached from a running jump | 3 above the floor |

The grid on the field is 8 units; objects snap to 0.25.
