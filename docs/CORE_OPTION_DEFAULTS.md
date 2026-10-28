# Core Option Default Labels

Source Review Date: 2026-10-28.

## RetroArch And Other Cores

Libretro separates the machine-readable option value, its display label and the default value. RetroArch uses the declared default for initialization and its per-option Reset action. In the inspected menu paths, the displayed current value and dropdown choices use the supplied value labels; no default suffix is added automatically. The dropdown checkmark identifies the current selection, which can differ from the default.

The six reviewed cores do not enforce one universal labeling convention:

| Core | Default Labels In Reviewed English Definitions |
| --- | --- |
| PCSX ReARMed | Uses `Libretro (Default)` for Memory Card 1 Type |
| Beetle PSX | Uses `(Default)` on selected numeric settings, including memory card indices; some labels use lowercase `(default)` |
| Snes9x | No `(Default)` value-label suffix found |
| Genesis Plus GX | No `(Default)` value-label suffix found |
| mGBA | No `(Default)` value-label suffix found |
| PPSSPP | No `(Default)` value-label suffix found |

The suffix is therefore an existing core convention rather than a required API feature. This fork consistently uses the requested Title Case `(Default)` form on every adjustable option.

Primary sources:

- [Libretro schema](https://github.com/libretro/libretro-common/blob/master/include/libretro.h): `retro_core_option_value::label` and `retro_core_option_v2_definition::default_value`.
- [RetroArch current-value rendering](https://github.com/libretro/RetroArch/blob/master/menu/cbs/menu_cbs_get_value.c): `menu_action_setting_disp_set_label_core_option`.
- [RetroArch dropdown rendering](https://github.com/libretro/RetroArch/blob/master/menu/menu_displaylist.c): `menu_displaylist_parse_core_option_dropdown_list`.
- [RetroArch Reset action](https://github.com/libretro/RetroArch/blob/master/menu/cbs/menu_cbs_start.c): `action_start_core_setting`, and [manager implementation](https://github.com/libretro/RetroArch/blob/master/core_option_manager.c): `core_option_manager_set_default`.
- Core definitions: [PCSX ReARMed](https://github.com/libretro/pcsx_rearmed/blob/master/frontend/libretro_core_options.h), [Beetle PSX](https://github.com/libretro/beetle-psx-libretro/blob/master/libretro_core_options.h), [Snes9x](https://github.com/libretro/snes9x/blob/master/libretro/libretro_core_options.h), [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX/blob/master/libretro/libretro_core_options.h), [mGBA](https://github.com/libretro/mgba/blob/master/src/platform/libretro/libretro_core_options.h), [PPSSPP](https://github.com/hrydgard/ppsspp/blob/master/libretro/libretro_core_options.h).

Downloaded source snapshots and SHA-256 hashes are retained in `build-macos-arm64/default-option-labels-2026-10-28/references/` and `references.json`.

## RPCS3 Behavior

Before each option registration, `libretro_option_labels.h` derives the marker from the published default. It handles all normal and advanced controls, visibility selectors and dynamic Patch Manager switches/configuration values. The single-value informational Patch Status row receives no suffix.

For example, SPU XFloat Accuracy displays **Approximate (Default)**, while the saved value remains `approximate`. No key, raw value, choice order or default is changed. Existing personal selections remain selected. The marker identifies the core option's declared default, not a recommendation for every game or an effective value overridden by the compatibility database.

Markers are derived after platform defaults and dynamic patch definitions are prepared. Repeated registration does not duplicate the suffix. If a published default changes, the former value loses the marker and the new value receives it. Localized value labels are retained with the same `(Default)` suffix; missing labels fall back to the English definition. The published English default determines the marker even if a generated translation still declares an older default. Generated translation files are unchanged.

API 2 and API 1 use value labels. API 0 has no distinct label field, so the option description receives `[Default: Approximate]` instead; the raw value list remains unchanged, with the default first as required by that API.

## Verification

The native option audit checks every published default label through API 2/1/0, including English/Italian fallback, non-first defaults, numeric and boolean synthetic patch options, a changed default, stable label storage, repeated registration and the unmarked informational status. It also compares the complete keys/defaults/raw-values contract before and after the change. This is synthetic registration evidence; no game or live RetroArch configuration is required by the audit.
