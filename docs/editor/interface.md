# Interface

*Русская версия — [interface.ru.md](interface.ru.md). Back to the [editor guide](README.md).*

```
+------+---------------------------------------------------------------+
| rail | [level card]                         [Undo] [Save] [Test]     |
|      |  name, author,      [inspector]       status message           |
|Select|  mode, limit        selected object                            |
|Hand  |                                                                |
|Area  |                  the field: the level from the side            |
|------|                                                                |
|Block |                                                                |
|Ramps |                                                                |
|Bag   | [top view: depth of the pieces .............................]  |
|...   |  hint line       [Shapes Graphics Objects Top  -  100%  +  Fit]  |
|Close |                                                                |
+------+---------------------------------------------------------------+
```

The field fills the window; the panels float over it. The rail on the left holds the tools.

## Tool rail

A tool decides what a left click on the field does. Placing tools show the object's outline under the
cursor; click to place it, `Esc` goes back to **Select**.

| Tool | Key | What it does |
|---|---|---|
| Select and drag | `V`, `Esc` | Click selects, drag moves. The default tool. |
| Hand | `H`, or hold `Space` | Drag moves the view; nothing is selected. |
| Area | `A`, or hold `Shift` | Drag a frame over the field; the [area bar](#area-bar) appears over it. |
| Block | `B` | A box you run and jump on. |
| Ramp to −x, Ramp to +x | — | A slope, high on the left or on the right. |
| Bag | — | A bag to collect (Bag collection mode only). |
| Checkpoint | — | A respawn point with its turning dish. |
| Zip line | — | A cable Faith slides down. |
| Swing bar | — | An invisible grab point: Faith hangs and swings. |
| Music cue | — | An invisible marker that switches the music. |
| Palette piece | — | A visible model from the level's chapter; opens the [palette](#palette). |
| ✕ (bottom) | — | Close the level. Unsaved changes are lost. |

Each object is described in [Objects](objects.md).

## The field

The level seen from the side: x to the right, y up. On the field:

- **Blocks and ramps** — filled shapes, coloured by [layer](objects.md#layers-zones-you-do-not-stand-on);
  a block with a surface shows the facade it gets in the game.
- **Palette pieces** — pictures of the pieces as the game draws them, each in its frame, far ones
  behind near ones. Hover a piece to see its full preview next to the cursor.
- **Objects** — start, finish, checkpoints, bags, swing bars, zip lines, sounds and music, with labels.
- **The grid** every 8 units, and two red lines — the left and right bounds of the level.

Moving the view (as in Figma):

| Mouse / keyboard | Touch |
|---|---|
| Wheel — scroll up and down, `Shift` + wheel — sideways | Two fingers — move |
| `Ctrl` + wheel — zoom at the cursor; `+` / `−` — zoom; `0` — fit the level | Pinch — zoom |
| Middle or right button drag, `Space` + drag, Hand tool | — |

### Selecting

- **Click** an object to select it. The [inspector](#inspector) opens next to it.
- **Click again** without moving: the next object under the cursor is selected — the one behind,
  then the one behind that, round and round. The inspector says how many are stacked there
  ("3 here, one behind another; this is #2").
- Press on the **selected** object and drag: it moves even if something else lies on top of it.
- **Ctrl + click** adds an object to the selection or takes it out. While `Ctrl` is held the inspector
  turns see-through and lets clicks pass, so it never covers the neighbour you want next.
- **Area → Select all inside** selects everything wholly inside a frame (works with fingers too).
- Drag any member of a group — the whole group moves (one undo step). Click one member without
  moving — only it stays selected. `Esc` clears the selection; `Del` deletes everything selected.

Order under the cursor: points of objects, sounds and zip lines first, then the finish, checkpoints,
piece points, blocks and ramps (the top one first), then pieces by their frame (the smallest first).

### Moving and resizing

Everything snaps to 0.25. A selected block, ramp, checkpoint or finish shows handles on its corners and
sides: drag an edge or a corner to resize. Everything is one `Ctrl+Z` away.

### Add here

A **right click** on the field (or **holding a finger** still for half a second) opens **Add here**: a
block, a ramp, a bag, a checkpoint, a zip line, a swing bar, music or a palette piece — put exactly at
that point.

## Level card

The panel at the top left; the arrow folds it.

- **In place of** — the base chapter or arena, and what the level is: *from scratch — can be uploaded*,
  *a chapter copy — stays on this PC*, *downloaded*, *uploaded*.
- **Name** and **Author** — shown in the level lists and online.
- **Mode** — **Race** (a start, a finish, optional checkpoints) or **Bag collection** with a **time
  limit** of 2, 3 or 5 minutes. A level on an arena base is bag collection only.
- **Visible blocks and ramps** — on: the game draws blocks and ramps as white shapes (a blockout you
  can play straight away). Off: only palette pieces and surfaces are visible — the level looks like
  the city. New levels from scratch start with it on, chapter copies with it off.
- A line saying what the mode needs.

## Undo, Save, Test

The panel at the top right.

- **Undo** (`Ctrl+Z`) — 200 steps.
- **Save** (`Ctrl+S`) — writes the level and checks it: a race needs a finish, bag collection needs at
  least one bag. If the game will not take the level, the message says why.
- **Test** (`F5`) — saves and starts the level in the game in its mode. On PC the game opens in its own
  window and the editor waits; on Android the editor closes and comes back to the same place.

Messages ("saved", "game started", what went wrong) appear under the buttons; ✕ hides one.

## Inspector

The panel next to the selected object; it places itself where it does not cover the selection. On top:
the object's name and the trash can (`Del`). Below: a preview (a piece, a bag, a checkpoint's dish),
an explanation, and the object's fields. Type a number or change it with − and +.

Buttons marked **i** open a short explanation (layer, material, swing bar height); a click elsewhere
closes it.

With a group selected the inspector shows **Selected: N**, what is in it, **Delete all** and **Clear the
selection**.

The fields of each object are in [Objects](objects.md).

## Area bar

Drag a frame with the **Area** tool or `Shift` + drag. Over the frame:

- its size;
- **Fill with selected** — fills the frame with copies of the selected block (at its size) or piece (at
  its frame), up to 400;
- **Remove tiles** — removes blocks and pieces wholly inside;
- **Select all inside** — selects everything wholly inside for moving as a group;
- **As in the game** — the game renders the area with its own camera, without Faith, and shows the
  picture in a window (on Android the editor closes for a moment and comes back);
- ✕ — clear the frame.

Filling and removing are one undo step.

## Top view

The strip at the bottom (button **Top**) shows the level from above: x as on the field, depth from front
(bottom) to back (top). The run line and the blocks are a band; pieces are rectangles coloured by
depth — background, run line, front. Click a piece to select it, drag it up or down to change its
depth (step 0.25). See [depth](objects.md#depth).

## View bar

At the bottom:

- **Shapes**, **Graphics**, **Objects** — show or hide blocks and ramps, palette pieces, objects. Hidden
  things cannot be selected.
- **Top** — the top view strip.
- **−**, zoom, **+**, **Fit** — zoom and fit the whole level.

A hint line on the left of it describes what the current tool does.

## Palette

The **Palette piece** tool opens the palette of the level's chapter: tiles with pictures, in folded
groups with the number of pieces — buildings (tops with a roof, corners and ends, sections and floors),
flat walls and signs, roofs, ledges and railings, frames, cranes and scaffolds, props, background, other.
Inside a group the pieces used most in the chapter come first. Pick a piece, then click the field.

The same palette opens for choosing a **surface** and for **Cover with a piece…**.

### Piece previews

The pictures are drawn by the game itself the first time a chapter is used and kept for later. While
they are being made, a bar shows the progress ("piece previews: 120 of 391") and tiles appear by
themselves. On Android press **Prepare piece previews**: the editor closes while the game draws them
and comes back to your level.

## Keyboard and mouse

| Key | Action |
|---|---|
| `V`, `Esc` | Select tool; `Esc` also clears a group selection |
| `H`, hold `Space` | Hand |
| `A`, hold `Shift` | Area |
| `B` | Block tool |
| Click, click again | Select, select the one behind |
| `Ctrl` + click | Add to / remove from the selection |
| Right click | Add here |
| `Del` | Delete the selection |
| `Ctrl+Z` | Undo |
| `Ctrl+S` | Save |
| `F5` | Test |
| Wheel, `Shift` + wheel | Scroll the view |
| `Ctrl` + wheel, `+`, `−` | Zoom |
| `0` | Fit the level |

## Touch

On a touchscreen (and always on Android):

- one finger — select and drag; tap again — the one behind;
- hold a finger still — **Add here**;
- two fingers — move and zoom the view;
- the frame icon on the rail (third) — the Area tool, drag a frame with a finger;
- **Undo** is the button next to Save and Test; panels and the palette scroll with a finger.
