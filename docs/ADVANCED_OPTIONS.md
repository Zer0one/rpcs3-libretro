# Advanced Option Visibility

Source Review Date: 2026-10-26. This refines the earlier category-only organization: a global visibility selector avoids separate standard/advanced CPU and GPU categories.

## What Existing Cores Actually Do

| Core | Real Selector | Visibility Mechanism | With V2 Categories |
| --- | --- | --- | --- |
| Snes9x | Show Advanced Audio/Video Settings | Shows/hides video layers, effects and audio-channel controls with `SET_CORE_OPTIONS_DISPLAY` | Selector hidden; categorized controls remain available |
| Genesis Plus GX | Show Advanced Audio Volume Settings | Shows/hides individual channel-volume controls, when compiled with per-channel configuration | Selector hidden; channel category supplies the hierarchy |
| PCSX ReARMed | Show Advanced P.E.Op.S. GPU Settings; Show Advanced UNAI GPU Settings | Shows/hides plugin-specific controls and registers a display-update callback for flat menus | Selectors hidden; plugin categories supply the hierarchy |

These are real core-defined options, not a special advanced flag in the Libretro schema. They affect menu visibility rather than resetting the underlying settings. Defaults for the cited selectors are disabled; availability also depends on build flags. This evidence establishes a recurring pattern, not a universal rule across every core.

Source locations:

- Snes9x: [definitions](https://github.com/libretro/snes9x/blob/master/libretro/libretro_core_options.h), [registration and visibility logic](https://github.com/libretro/snes9x/blob/master/libretro/libretro.cpp), especially `retro_set_environment` and `update_variables`.
- Genesis Plus GX: [definitions](https://github.com/libretro/Genesis-Plus-GX/blob/master/libretro/libretro_core_options.h), [registration and visibility logic](https://github.com/libretro/Genesis-Plus-GX/blob/master/libretro/libretro.c), especially `USE_PER_SOUND_CHANNELS_CONFIG`, `retro_set_environment` and `check_variables`.
- PCSX ReARMed: [definitions](https://github.com/libretro/pcsx_rearmed/blob/master/frontend/libretro_core_options.h), [registration and visibility logic](https://github.com/libretro/pcsx_rearmed/blob/master/frontend/libretro.c), especially `update_option_visibility` and `retro_set_environment`.
- [Libretro API](https://github.com/libretro/libretro-common/blob/master/include/libretro.h): `RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY` and `RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK`.

The retrieved source copies, URLs and SHA-256 hashes are retained in `build-macos-arm64/advanced-option-selector/reference-manifest.json` and `references/`.

## RPCS3 Behavior

- **Show Advanced Options** is a root Core Option, saved as `rpcs3_show_advanced_options`, with **Disabled** as its default.
- It works with V2 categories as well as flat menus. This is a deliberate adaptation of the established mechanism to the requested interface, rather than a claim that all reference cores keep the toggle visible with categories.
- Each function has one category: System, CPU, GPU, Scheduling, Timing, Audio, Network, On-Screen Display and Diagnostics. CPU/GPU no longer have separate advanced counterparts.
- Enabling the selector reveals 70 existing controls on macOS, 69 elsewhere, inside those same groups. The full [option inventory](CORE_OPTIONS_REVIEW.md#fixed-option-inventory) marks advanced entries.
- The selector remains visible so the user can turn it back on. Basic controls and Patch Manager are independent of it.
- Hiding controls preserves their saved values and effects. Core option application and the game database still work as before. A hidden non-default setting stays active; use the frontend's reset-options command if defaults are wanted.
- All fixed definitions remain registered. Existing keys, allowed values and emulation defaults are unchanged; the only added key controls visibility.
- This is separate from RetroArch's global Show Advanced Settings preference. No frontend configuration is rewritten.

## Update And Fallback Behavior

The display callback is registered even before a game has patches. It updates menu visibility without needing an emulation frame, so it can work while the Quick Menu pauses content. The core also updates visibility when frontend variables change during `retro_run`, for frontends without that callback.

Visibility is forcibly reapplied after each option-list publication, including game-load/patch-list changes. Early unsuccessful environment calls are retried rather than latching an incomplete state. Patch Manager keeps its existing dependent visibility callback through the combined update path.

If a frontend does not support per-option visibility, the request may be ignored and all definitions remain accessible. Empty-category handling and immediate menu refresh are frontend behavior; reopen Core Options if necessary. The selector does not require restarting RetroArch or resetting the game.

## Verification

Native schema and mock-environment checks verify API 2/1/0 registration, root-selector reachability, functional grouping, English/Italian fallback, disabled default, show/hide/repeated calls, forced republication and recovery after early unsupported display calls. Saved-contract comparison verifies all 98 previous macOS keys, value lists and defaults are unchanged. The only new option is the selector.

The macOS ARM64 build and strict signature/dependency checks are separate from actual menu-rendering evidence. No emulator or game is launched by these tests; real frontend rendering and other-platform binaries remain unverified by this activity.
