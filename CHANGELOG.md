# Changelog

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
