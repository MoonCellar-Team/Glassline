# Levels

*Русская версия — [levels.ru.md](levels.ru.md). Back to the [editor guide](README.md).*

## Creating a level

**Create level** on the launcher's Levels tab opens the editor's start page: your levels with **Open**, and
**New level**:

- **id** — the folder name of the level: latin letters, digits, `-` and `_`;
- **Base** — a chapter of the story or a BAG HUNT arena (Steps, Joust, Pinball, Maze). The base gives the
  level its palette of pieces, its models, sky and lighting, and the game runs the level in its place;
- **Create from scratch** — a floor, a start and a finish (on an arena: a floor, a start and the first
  bag). Such a level is all yours and **can be uploaded**;
- **Chapter copy** / **Arena copy** — the whole map of the base, to change as you like. It carries EA's
  layout, so it **stays on this device**: it cannot be uploaded or sent to a friend.

### Modes

| Mode | Needs | How it plays |
|---|---|---|
| Race | a start and a finish; checkpoints are optional | a time trial to the finish |
| Bag collection | at least one bag; no finish | collect bags until the time limit (2, 3 or 5 minutes) runs out or all are taken |

Switch the mode in the level card. A level on an arena base is bag collection only.

## Saving and testing

- **Save** (`Ctrl+S`) writes the level and checks it the way the game will. A message says what is wrong
  ("saved, but the game will not take it: …").
- **Test** (`F5`) saves and runs the level in the game, in its mode, with a quick start and exit. Close the
  game (or quit to the menu on Android) to come back to the editor at the same place.
- **As in the game** on an [area](interface.md#area-bar) shows a part of the level rendered by the game
  without running it.

Your own best time (and best bag count) is kept with the level.

## The Levels tab of the launcher

The header shows **My levels · N** with **Import level…** and **Create level**. Each level is a row:

- its name and *id · mode*;
- a coloured label of its state (hover it for the meaning):

  | Label | Meaning |
  |---|---|
  | online | uploaded and the same as here |
  | changed here | your edits are not online yet — **Update online** |
  | newer online | the online version is newer — **Take from server** in the ⋯ menu |
  | not uploaded | only on your devices |
  | removed or hidden | not on the server: unpublished elsewhere or hidden after reports |
  | hidden after reports | online, but hidden until a moderator looks at it |
  | other account | uploaded from another account; sign in with it to update or unpublish |
  | downloaded | someone else's level from the server |
  | chapter copy, arena copy | carries EA's layout — stays on this device |
  | from a partner | came over the network in a game together — cannot be uploaded |

- **Edit** — open it in the editor;
- **Upload** / **Update online** — share it, or send your edits (needs a MoonCellar account on the
  Account tab); copies show *local only* instead;
- **⋯** — **Take from server…** (replace this copy with the online version), **Unpublish…** (remove it
  from the server: its leaderboard and ratings go, your copy stays), **Export to a file**, **Delete…**.

Unpublish, take and delete ask first. Deleting removes the level from this device and from your account,
so from your other devices too; an uploaded level stays online until you unpublish it.

### Files and other devices

- Your levels live in the `levels` folder of your data folder (see *Your data* in [How to play](../../README.md#how-to-play)), one
  folder per level with `level.bin` and `level.json`.
- Signed in, your levels sync through your account to your other PCs and phones.
- **Export to a file** writes the level as one `.glevel` file (on Android — to Downloads). **Import
  level…** or dropping a `.glevel` file on the launcher window adds it; a level with the same id gets a new
  one.

## Playing levels

- **EXTRAS → PLAYER LEVELS** in the game's main menu: your levels and **ONLINE** — the shared ones.
  Filter by mode, sort (new, best), search; **PLAY** downloads and starts a level; **RATE** (1–5) and
  **REPORT**; your own can be uploaded, unpublished or deleted there too.
- Bag collection levels are also listed in **BAG HUNT** after the four arenas.
- Uploaded and downloaded levels have their own online leaderboard with ghosts, and you can challenge a
  MoonCellar friend on them.
- In a game together (split screen or online) the host can pick a player level; the other player receives
  it automatically.

## Tips

- Build the route with plain blocks first, play it with **Visible blocks and ramps** on, and dress it with
  pieces only when it plays well.
- Check the sizes against Faith's moves (see [the table](README.md#sizes-to-build-with)).
- Put a checkpoint before every hard part.
- Use **As in the game** often: the field shows pieces flat, the game shows depth and light.
