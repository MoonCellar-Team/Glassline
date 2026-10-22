# port-ui (mcui)

The MoonCellar look for the game ports of MoonCellar Team: a small C++ library over
[Dear ImGui](https://github.com/ocornut/imgui) (1.92 or newer) that the port's launcher and its
overlay over the game draw with. It is not built here — no package of this monorepo uses it —
but copied into each port (see "In a port").

## What is in it

| File | What |
| --- | --- |
| `src/mcui_tokens.h` | The site's tokens as C++ constants: colours, radii, spacing, durations. Generated, never edited |
| `src/mcui.h`, `src/mcui.cpp` | Fonts, the ImGui style from the tokens, text, buttons, the toggle, the segmented tabs, the progress bar, the panel |
| `src/mcui_icon.cpp` | Outline icons: the 24×24 SVGs of the ports' achievements and the site's icon style, parsed once and drawn in any colour |
| `src/mcui_widgets.cpp` | The achievement card, the device-code sign-in, the notifications |
| `sync-tokens.ts` | Writes `src/mcui_tokens.h` from `docs/design-tokens.css`; `--check` only compares |

The mockup of these components is `docs/mockups/port-ui.html`.

## Rules

- **Tokens come from the site.** `src/mcui_tokens.h` is generated from `docs/design-tokens.css`,
  the snapshot `bun --filter web sync:tokens` writes from `apps/web/src/lib/app/styles/vars`.
  After a token changes: `bun --filter web sync:tokens`, then `bun packages/port-ui/sync-tokens.ts`.
  `bun packages/port-ui/sync-tokens.ts --check` exits 1 when the header is stale.
- **Controls look as the site draws them** in Storybook (`docs/design-system.md`): buttons
  (`Button`: tertiary fill, accent for the main action, `--radius-button`), the toggle
  (`ToggleSwitch`: red knob on the left when off, green on the right when on), segmented tabs
  (`Tabs`), panels (`Box`: `--color-bg-secondary`, `--radius-x5`), notifications (`Toast`:
  `--color-bg-primary`, `--radius-x2`). A new component gets a mockup here first.
- **No font files.** The site's face, Apercu Pro, is a licensed file; a port keeps its own copy
  under its licence and passes the paths to `mcui::LoadFonts`. Faces it lacks fall back to the
  first `fallback` file (a system font), so the library still works without them.
- **No icon library.** Icons are 24×24 outlines (stroke 1.8, round caps and joins, paths only),
  like the site's `shared/ui/svg` and the ports' achievement icons.
- **No backend.** The port brings the ImGui platform and renderer backends and the textures (the
  `logo-icon` mark for notifications, `mcui::SetNotificationMark`).

## In a port

1. Copy `src/` into the port (`port/third_party/mooncellar-ui/` in the first port) and build the
   three `.cpp` files with Dear ImGui; note the commit it came from.
2. Before the first frame: `mcui::LoadFonts({regular, medium, bold, fallback})`, then
   `mcui::ApplyStyle(scale)`.
3. Draw with ImGui as usual: the style already makes ImGui's own widgets MoonCellar; the components
   (`mcui::Button`, `mcui::Toggle`, `mcui::Segmented`, `mcui::BeginPanel`, `mcui::AchievementCard`,
   `mcui::DeviceCode`) cover what plain widgets do not. They work with keyboard and gamepad
   navigation as well as the mouse: the arrows or the D-pad reach every segment and cross into a
   panel, and `mcui::Segmented(..., true)` also switches segments with the gamepad's shoulder
   buttons (LB and RB), keeping the focus on the chosen one.
4. Over the game: `mcui::Notify` from any thread, and every frame while
   `mcui::NotificationsActive()` — an ImGui frame with `mcui::DrawNotifications(now)`.

The code is MoonCellar's, under the repository's licence; ports of MoonCellar Team use it with the
owners' consent.
