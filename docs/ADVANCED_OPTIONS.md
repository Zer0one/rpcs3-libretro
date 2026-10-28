# Advanced Option Visibility

Source Review Date: 2026-10-26. Updated 2026-10-28: independent section-local selectors replace the global visibility control, while keeping one category per function.

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

- Every adjustable normal or advanced option marks its declared default value with **(Default)**. The suffix changes presentation only; platform defaults, dynamic patches, translations and API 0 fallback are covered by the [default-label convention](CORE_OPTION_DEFAULTS.md).
- **Show Advanced Options** appears immediately before the advanced block inside CPU and GPU. These mixed sections keep their normal controls visible and independently show/hide their advanced controls. Saved keys: `rpcs3_cpu_show_advanced_options`, `rpcs3_gpu_show_advanced_options`.
- **Show Advanced Sections** is a root control after every normal section, including Patch Manager, and before the dedicated advanced sections. Its key is `rpcs3_show_advanced_sections`; it defaults to **Disabled**.
- The root gate shows/hides **Scheduling (Advanced)**, **Timing (Advanced)** and **Diagnostics (Advanced)** together. These sections are last and contain no internal visibility selector. The gate never changes CPU/GPU visibility.
- Both CPU/GPU selectors also default to **Disabled**. Every visibility control remains reachable when its targets are hidden, in categorized V2 and flat API 0/1 menus.
- The former root `rpcs3_show_advanced_options` key is removed; its saved value is no longer used. New visibility preferences initially use their disabled defaults. No emulation value is reset.
- Existing classification is preserved: 70 advanced controls on macOS, 69 elsewhere. System, Audio, Network and On-Screen Display acquire no internal selector. No duplicate standard/advanced categories are added, and raw category keys are unchanged.
- Hiding controls or sections preserves saved values and effects. Core option application and the game database still work as before. All definitions remain registered; these preferences are independent of RetroArch's global Show Advanced Settings preference.

## Patch Manager Availability

Patch Manager is always registered before the root gate. A completely empty category is hidden by RetroArch, as shown by its [category-visibility check](https://github.com/libretro/RetroArch/blob/7c16e4e49d85b3ec3dacf458fb2fa6c642132b85/core_option_manager.c#L1498) and [menu category handling](https://github.com/libretro/RetroArch/blob/7c16e4e49d85b3ec3dacf458fb2fa6c642132b85/menu/menu_displaylist.c#L16922). The core therefore supplies a single-value informational Patch Status entry when there are no real patch controls:

- No title loaded: **Load a title to check for patches**.
- Loaded title with no matching listed patches: **No patches available for this title**.

This is diagnostic output from the title/catalog state, not an adjustable patch setting or a claim that the game is unsupported. Real patch controls replace the status row when available. State is refreshed on title collection and reset on unload/deinitialization. API 1/0 conversion uses the same published definitions as V2, so per-title patches and the informational row are not lost on older frontends.

## Update And Fallback Behavior

The display callback is registered even before a game has patches. It updates menu visibility without needing an emulation frame, so it can work while the Quick Menu pauses content. The core also updates visibility when frontend variables change during `retro_run`, for frontends without that callback.

Visibility is forcibly reapplied after each option-list publication, including game-load/patch-list changes. Patch Manager is independent of every visibility control; the three dedicated advanced sections follow only the shared root gate. Early unsuccessful environment calls are retried per section rather than latching an incomplete state. A failed section does not cause successful sections to replay their visibility updates. Patch Manager keeps its existing dependent visibility callback through the combined update path.

If a frontend does not support per-option visibility, the request may be ignored and all definitions remain accessible. Empty-category handling and immediate menu refresh are frontend behavior; reopen Core Options if necessary. The selector does not require restarting RetroArch or resetting the game.

## Verification

Native schema and mock-environment checks verify API 2/1/0 registration, contiguous functional groups, a reachable selector immediately before its target block in each mixed section, a root section gate after Patch Manager, no selector inside dedicated advanced sections, English/Italian fallback and disabled defaults. They exercise independent toggles, stable repeated calls, forced republication and section-local recovery after unsupported display calls. Saved-contract comparison verifies all prior emulation keys, value lists and defaults remain unchanged; only the menu visibility keys change. Native tests also check advanced-only suffixes and final ordering with a synthetic game-dependent Patch Manager category, both root-gate directions, and no-title/no-patch informational registration through API 2/1/0.

The macOS ARM64 build and strict signature/dependency checks are separate from actual menu-rendering evidence. No emulator or game is launched by these tests; real frontend rendering and other-platform binaries remain unverified by this activity.
