# Objects

*Русская версия — [objects.ru.md](objects.ru.md). Back to the [editor guide](README.md).*

Every object is added the same three ways:

- pick its tool on the rail and click the field;
- right click (or hold a finger) on the field → **Add here** → the object;
- select one that is already there and use the [area bar](interface.md#area-bar) → **Fill with selected**
  (blocks and pieces).

Then select it to see and change its settings in the inspector. `Del` or the trash can deletes it.

## Blocks and ramps

What Faith runs, jumps and climbs on. **Block** (`B`) is a box; **Ramp to −x** is high on the left,
**Ramp to +x** is high on the right. A new block is 4 × 1, a new ramp 4 × 2; drag the edges to resize.

| Field | Meaning |
|---|---|
| shape | block, ramp to −x, ramp to +x |
| layer | what the shape is for the game (see [layers](#layers-zones-you-do-not-stand-on)) |
| material | the footstep sound (see [materials](#materials)) |
| min x, min y, max x, max y | the box of the shape |

Whether the game shows them is set in the level card (**Visible blocks and ramps**): on — white shapes,
off — invisible, and you see only palette pieces and surfaces. The inspector reminds you which, and lists
the palette pieces that cover the shape ("Covered by pieces: …") so you can check nothing is bare.

To make a block look like the city there are two ways.

### Surface

A **surface** dresses the whole shape with one palette piece: its facade is repeated over the front, its
roof over the top (over the slope of a ramp), its sides over the sides, cut at the edges; a little of what
stands above the piece's roof (a parapet, vents) stays, up to 2.5. Good for buildings and facades with a
flat roof.

Select a block or a ramp → **surface: Choose…** in the inspector (or the round button by the block's
corner) → pick a piece. Only pieces with a facade are offered. **Change…** picks another, **Remove** takes
it off. On the field the block shows the facade it will get.

### Cover with a piece

**Cover with a piece…** (blocks only) puts copies of one piece in a row along the top of the block, its
whole length: the top of the piece at the top of the block, one copy per piece width — floors, roofs,
ledges. When the piece's side is open, the row gets an end piece with a closed side. If the piece has no
roof (you would see through it from above) the status line warns about it. One undo step removes the row.

### Layers: zones you do not stand on

Only layer 0 is solid. Layers 1–3 are zones: Faith passes through them, the game does not show them, and
they change what she does while inside. The **i** next to the layer explains each.

| Layer | What it does | How to use |
|---|---|---|
| 0 solid | Run, jump, bump into it | Everything you stand on |
| 1 wall run | A running jump while Faith is in the zone becomes a run along the wall behind her (up to a second) | About 10 × 3.5 over a gap, in front of a wall or a sign in the background, at running height; no solid wall is needed |
| 2 balance beam | Running in the zone, Faith walks a balance beam | Around a narrow solid beam: 0.5 below to 0.5 above its top |
| 3 long jump | A jump from the zone is a long one: faster and higher | About 2 × 0.6 on the very edge of a roof, its bottom on the roof |

Zones have no surface and no **Cover with a piece**.

### Materials

The material only picks the footstep sound; running and jumping are the same on all of them.

| Material | Sound |
|---|---|
| 0 | concrete |
| 1 | metal duct |
| 2 | wooden planks |
| 3, 5 | metal gantry |
| 4 | crane metal |
| 6 | concrete; the shape is visible as a dark shape even with Visible blocks and ramps off |
| 7 | concrete; visible as a white shape even with Visible blocks and ramps off |

For a wall-run zone the material is the sound of her steps on the wall. For balance and long-jump zones it
changes nothing.

## Palette pieces

The scenery: buildings, walls, signs, cranes, pipes, props — the models of the level's chapter. Pieces are
looks only: Faith does not stand on them, she stands on the blocks under them.

Pick **Palette piece** on the rail, choose a tile in the [palette](interface.md#palette), click the
field. A new piece is placed on the run line.

| Field | Meaning |
|---|---|
| x, y | where the piece stands |
| depth | **Back**, **Run line** or **Front** — see below |
| z | depth as a number |
| Mirror left to right | flips the piece in place (**Mirrored — undo** turns it back) |

### Depth

Faith runs in the plane z = 0; blocks reach from −9 (back) to 1 (front).

- **Back** — the front edge of the piece at −10: behind the blocks, never covers Faith, nothing to run on.
- **Run line** — the front edge at 1, flush with the front of the blocks: roofs and floors line up with
  what she runs on. This is how the chapters place most pieces.
- **Front** — the back edge at 2: in front of the blocks, nearer than Faith, may cover her.

The [top view](interface.md#top-view) shows every piece's depth; drag a piece there to move it in depth.

Mirroring only changes the look: the blocks you run on stay as they are. Older versions of Glassline do
not show mirrored pieces.

## Start

Where Faith begins. Fields: x, y and **facing right**. A level always keeps one start; to move it, drag
it.

## Finish

The end of a race: run into the box. Fields: x, y, width, height (or drag its edges). A race keeps at
least one finish; a bag collection level needs none.

## Checkpoints

A box that saves Faith's progress: after a fall she respawns there. Every checkpoint has a **satellite
dish** — when Faith runs through, the dish turns towards her side (it takes about 0.75 s), as in the
chapters.

- A new checkpoint gets its own dish at its lower left corner (1 to the right, 0.5 up), made from the
  dish of the first checkpoint. In a level that has none yet, the size and the dish come from the base
  chapter.
- The inspector shows which dish the checkpoint turns. If it turns a dish far away or one shared with
  another checkpoint (older levels did that), press **Own dish here**: a dish piece standing next to the
  checkpoint becomes its dish, otherwise a new one is placed.
- **respawn facing right** — where Faith looks after respawning there.
- The dish moves and is deleted together with its checkpoint. While a checkpoint uses a dish, the dish
  cannot be deleted on its own.

Fields: min x, min y, max x, max y. The usual size is 1.5 × 7.

## Bags

Things to collect in **Bag collection** mode: the run lasts until the time limit or until every bag is
taken. A race has no bags (the inspector says so). Fields: x, y. A bag is about 1 × 1; Faith reaches one
up to about 3.6 above the floor in a jump.

## Swing bars

An invisible grab point. When Faith jumps into it she hangs 1.6 below and swings; tilting (the tilt keys
or the accelerometer) swings her harder. A jump or a forward swipe as she swings forward launches her
forward and up (7.5 forward, 5 up).

The game does not draw the bar: put a palette piece through the point — a pipe, a beam, scaffolding. On the
field a swing bar is a short bar with a line down to where Faith will hang. The **i** in the inspector
gives the heights: from a running jump on flat ground she reaches a bar 3 above the floor; in the chapters
bars also hang 6–8 above a gap and are reached from a higher edge.

## Zip lines

A cable Faith grabs and slides down. A new one runs 16 to the right and 4 down. Drag either end, or set
**x1, y1** (start) and **x2, y2** (end) and **end offset** — how far before the end she lets go.

## Music and sounds

- **Music cue** — an invisible marker: when Faith reaches it, the chosen **Track** plays. Tracks are
  listed by their names in your copy of the game.
- **Ambient** and **one-shot sounds** — markers that come with chapter copies; choose another sound or
  move them.

## Other objects of chapter copies

A copy of a chapter keeps its own objects: pigeons, the chopper, police, riot police, rivals, plane
lights. They can be moved and deleted, not added.

## Examples

### A gap to jump

1. Two blocks with tops at the same height, 5 apart (Faith's running jump is about 7; 5 is comfortable).
2. Test (`F5`). Wider than 7 needs a long-jump zone (below) or a swing bar.

### Steps up

Blocks 1 high are taken without a jump; a ledge up to 2 above her feet is climbed from a jump. A
staircase of blocks 2 apart in height is a reliable climb.

### A block that looks like a building

1. A block for the building, its top where the roof should be.
2. **surface: Choose…** → a building piece with a flat roof.
3. Turn **Visible blocks and ramps** off in the level card, press **As in the game** over the area to see
   the result.

### A billboard you can run along

A billboard is a palette piece, so Faith falls through it unless a block is under it. The chapters put a
block whose top is exactly at the top edge of the billboard:

1. Place the billboard piece (for example a 10 × 2.3 sign).
2. Place a block as wide as the billboard, its **max y** at the top of the billboard (see the piece's frame
   on the field). The block can go down to the roof below, so there is no gap to fall into.
3. **material 2 wooden planks** — the sound the chapters use there.

### A wall run

In the side view a wall run goes along a wall *behind* Faith — a billboard or the side of a building in the
background — over a gap.

1. A gap between two roofs, and a wall or sign piece in the background across it.
2. A block with **layer 1 wall run** over the gap, about 10 long and 3.5 high, starting at the edge, its
   bottom about 1.5 above the roof (where Faith is when she jumps).
3. Run, jump at the edge: Faith runs along the wall for up to a second and lands on the far roof.

### A balance beam

1. A narrow solid block as the beam, 0.5 thick, dressed with a pipe or a beam piece.
2. Around it a block with **layer 2 balance beam**, as long as the beam, from 0.5 below its top to 0.5
   above (1.5 high in all).

### A long jump over a big gap

A small block with **layer 3 long jump** (about 2 × 0.6) on the very edge of the roof, its bottom on the
roof. The chapters also put a small ramp of the same size there as a kicker. Jumping from it sends Faith
faster and higher than a normal jump.

### Swinging over a gap

1. A gap 10–12 wide between two roofs.
2. A swing bar about 3 above the roof in the middle of the jump, or two bars 6 apart for a wider gap.
3. A pipe or scaffolding piece through the bar so players see it.
4. Test: run, jump, and when she swings forward jump again.

### A checkpoint with a turning dish

1. **Checkpoint** tool → click just before a hard part. It comes with its dish.
2. Check **respawn facing right** matches the direction of the run.
3. To use a dish you placed yourself: put the checkpoint next to it and press **Own dish here**.

### Moving a whole part of the level

1. **Area** (`A`) → drag a frame around the part → **Select all inside**, or `Ctrl` + click the pieces
   one by one.
2. Drag any of them — everything moves together; `Ctrl+Z` undoes the whole move.

### A long roof made of one piece

Select a roof piece, drag an area as long as the roof with the Area tool, **Fill with selected**. Or select
the block under it and **Cover with a piece…**.

### Reaching something hidden behind

Click the front object, then click the same spot again: the one behind is selected. Keep clicking to go
through the stack.
