# Changelog

## 1.3.1 — 2026-10-26

- **Android launcher:** reduced interface scale and added a fixed Play footer. Settings scroll above it, keeping the last item accessible.
- **Android level editor:** compact mobile layout with a collapsed level card, toolbars that fit the screen, bounded panels, and a smaller top view.
- **Launcher:** combined Levels help into one info button beside the heading. Game-copy help buttons stay beside Browse and Download instead of overlapping the game status.
- **Fixed:** entering browser fullscreen now starts the game on the first Play press, including when the canvas resizes.

## 1.3 — 2026-10-26

- **Browser version:** play with your own iPad game copy, stored locally in the browser. Includes audio, MoonCellar services, skin previews, the level editor, and online races with desktop players.
- **Mobile browsers:** touch and tilt controls, gesture-based help, a compact launcher with an accessible Play footer, and a smaller level editor. Added orientation selection and browser fullscreen where supported.
- **Launcher:** redesigned Heroine cards show custom skins’ cloud status. Added info tooltips and improved keyboard navigation, focus outlines, Esc handling, and Ctrl+Tab switching.
- **Gameplay:** combat slow motion can now be switched off on the Game tab.
- **Graphics:** floor reflections work again under Wine and Proton. The sky fills ultrawide screens without exposing the backdrop’s edges.
- **Player levels:** explicit star ratings with confirmation, clearer server errors, readable Russian labels, and easier page navigation. Players cannot rate their own levels.
- **Cloud skins:** additional skins can stay local when cloud storage is full. Resetting cloud storage no longer deletes local skins.
- **Controls:** added an on-screen pause button for touch input. Improved running hints, gamepad dismissal of level hints, and mouse dragging of scrollbars.
- **Fixed:** camera drift after hard landings, combat and final-boss freezes, and bouncing by the door in 5-2 at high frame rates.
- **Fixed:** network players and hazards remain still until both players are ready and the countdown ends, preventing movement and deaths while waiting.
- **Fixed:** crashes when quitting during startup or level loading, and an invisible focus target on the Wallpapers screen.
- **Security:** updated bundled network, image, and font libraries on Linux and Android; restricted decoding of untrusted image formats.

## 1.2 — 2026-10-24

- **Network play:** a takedown on your opponent now shows at once — you hear the hit and the opponent
  falls where you struck, instead of a moment later when you had already run past. While you are
  connected to a friend, Alt+Tab, minimizing the window or the pause menu no longer stop the game, so the
  start countdown and the race stay in step for both players.
- **Network play:** the lobby shows your ping to the MoonCellar server and to your friend, and the same
  two lines are in the top-left corner during a race or a Rivals game. After a Rivals game the host can
  RESTART the arena or SELECT the next one, and the friend follows; the selection window opens on the
  last mode, level and settings.
- **Fixed:** the heroine's footsteps, breathing, jumps and slides and the bag pickup sound could fall
  silent until you quit the game, and some sounds dropped after Alt+Tab.

## 1.1 — 2026-10-24

- **Russian:** the game can now be played in Russian — a full translation of the game's texts and of
  the port's menus, which the iPad original never had. Choose it on the launcher's Game tab or in the
  game's Language window; on a Russian system it is picked automatically the first time.
- **Languages:** on Windows and Android the launcher and the game's "As in the game" language now follow
  the system language, as on Linux. Menus in French, German, Italian and Spanish show accented capital
  letters (É, Ü, Ñ…) instead of lowercase ones.
- **Graphics:** floor reflections now cover the whole floor, without the torn white silhouette next to
  the runner. Ambient occlusion no longer draws dark stripes on distant buildings on AMD GPUs (Steam
  Deck). Under Wine and Proton floor reflections are switched off: Wine's shader compiler made them
  empty and very slow — on Linux and Steam Deck use the Linux build.
- **Defaults:** a new install starts in borderless fullscreen with MSAA off; saved settings stay as they
  are.
- **Fixed:** on Windows, launcher settings (graphics and everything else) were saved only the first
  time; a window popped up on every launcher start while skin and editor previews were redrawn; HD
  textures could not be generated again over earlier ones.
- **Fixed:** the update check no longer skips a new release silently when GitHub's request limit is
  reached (for example behind a shared VPN).

## 1.0.1 — 2026-10-22

- **Android:** the APK is now part of every release — `glassline-android-1.0.1.apk`, Android 9 or newer,
  ARM. It installs over an earlier Glassline APK from MoonCellar Team and keeps your data.
- **Updates:** if you run 1.0 as the AppImage or from the Windows zip, the launcher offers this version
  on start — press **Update**.

## 1.0 — 2026-10-22

First public release of Glassline — an unofficial port of Mirror's Edge for iPad (2010) to Linux, Windows
and Steam Deck. No game files are included: you need your own copy of the iPad version (1.1.48 or newer),
which you choose in the launcher.

- **Graphics:** any resolution and aspect ratio up to 21:9, rendering at your screen's resolution or
  upscaled (FSR 1.0), 30 to unlimited fps, MSAA and TAA, an optional remastered look with ambient
  occlusion, god rays, reflections and shadows — or the exact iPad picture with one switch.
- **Controls:** keyboard and gamepad everywhere, with rebinding; Steam Deck Game Mode works.
- **Network play:** one-on-one Race and Rivals with a four-character room code, no port forwarding;
  split screen on one PC.
- **MoonCellar online services** in place of EA's: sign-in with a code, leaderboards with ghosts,
  challenges to friends, cloud saves, 28 achievements.
- **Heroine skins:** your own textures and 3D models (glTF, GLB, VRM).
- **Launcher:** your game copy checked at once, all settings, and updates: when a new release is out the
  launcher shows what's new, and **Update** installs it after checking its signature (AppImage, Windows).
- **Player levels:** a side-view level editor, an online level browser with ratings and a leaderboard
  for every shared level. New: the [level editor guide](docs/editor/README.md)
  ([по-русски](docs/editor/README.ru.md)).
- **Languages and audio:** all 8 languages of the game, a launcher in English and Russian, headphone
  surround, your own music folder.

Everything in detail, known limitations and how to play: [README](README.md) ([по-русски](README.ru.md)).
Android builds will come later.
