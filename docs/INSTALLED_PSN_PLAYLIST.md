# Installed PSN Playlist

The core can maintain `Sony - PlayStation 3 (PSN Installed).lpl` after a successful base PS3 PSN PKG installation or when reopening a PKG that resolves to an existing installation. **System → Installed PSN Playlist** defaults to Enabled as a port convenience requested for this fork; standalone RPCS3 has no corresponding frontend-playlist option. Disabling it prevents all playlist reads and writes by this feature. It is independent of PKG Launcher.

## RetroArch Reference

The implementation follows [RetroArch's playlist writer and push/update behavior](https://github.com/libretro/RetroArch/blob/b77c31fbed046041cf7f2483b2b732e82bc59d1c/playlist.c), specifically JSON version 1.5, item/core/database fields, avoiding duplicate paths, preserving unrelated metadata and writing a temporary file beside the destination before replacement. This reference is an integration model, not a direct dependency on RetroArch's internal playlist functions.

Libretro exposes the frontend's playlist directory through `RETRO_ENVIRONMENT_GET_PLAYLIST_DIRECTORY` and the loaded core through `RETRO_ENVIRONMENT_GET_LIBRETRO_PATH`. It does not expose `playlist_push` or `playlist_write_file` as core environment calls. The core therefore uses the reported directory, its existing yaml-cpp dependency to read JSON, a JSON-compatible emitter and RPCS3's `fs::pending_file` for atomic replacement. No additional downloaded library or runtime network access is needed. Missing directory/core-path support causes a logged notification rather than a guessed output location.

## Installed Entries

A successful installation or a reopened PKG, followed by positive base-game identity and installed-file checks, can add a title. Updates, DLC, other package types, failed/incomplete installs and direct game launches do not add entries. Existing installations are not scanned automatically; reopen each base-game PKG to add it. With PKG Launcher enabled, recognized installed games take the launcher path and are not reinstalled. If a same-name unmarked list was removed, reopening the PKG creates the marked list and adds that title. The target respects the active virtual HDD mapping and is the actual installed `PARAM.SFO`, with a matching `USRDIR/EBOOT.BIN` required. The displayed label is the opened PKG's basename without its final `.pkg` extension, not installed `TITLE` or `TITLE_ID`. Case, punctuation, Unicode and region tags are retained. Native paths accept either separator and case-insensitive PKG extensions. For frontend URI paths, the frontend's canonical extension-free filename is used when provided by `GET_GAME_INFO_EXT`.

Entries associate with the loaded RPCS3 core and use the standard `Sony - PlayStation 3.lpl` database name. The source PKG can subsequently be moved or removed. Launching PARAM.SFO has no source PKG directory: RAP lookup uses the central collection or existing HDD-user `exdata`. The feature does not copy licenses or artwork.

New entries are appended; matching installed paths are not duplicated. The ordinary add/update operation refreshes the matching item's label from the current PKG filename; unrelated entries, item metadata, order and top-level settings remain intact. For example, opening `Daytona USA (Europe).pkg` labels that installed-game entry `Daytona USA (Europe)`. There is no separate correction routine, scan, old-label detection or one-time state. Reopening the same filename is a no-op when the entry is already current. A matching RPCS3 entry can refresh its core path when the core moved. Existing content paths are not rewritten after moving the virtual HDD; reinstalling into a different HDD location is a different path. The frontend may require returning to the playlist list or restarting to discover a new file.

## Ownership And Failure Protection

New lists carry this root-level JSON marker:

```json
"rpcs3_installed_playlist": "rpcs3-libretro:psn-installed:1"
```

An existing same-name list is updated only when it carries that exact marker. Otherwise the core preserves the file and reports the collision on screen after installation or on reopening a recognized PKG. It does not claim ownership based on the filename, contents or core association alone, and it does not silently choose another name or amend other playlists.

RetroArch's current writer does not preserve arbitrary extra root fields. If RetroArch or another editor rewrites the list and removes the marker, the core subsequently treats it as unowned and preserves it with a notification. There is no automatic ownership recovery: that conservative behavior protects a user-created same-name list.

The updater accepts uncompressed JSON version 1.5 only. Legacy line-format, compressed, malformed, oversized (over 8 MiB) or unreadable lists are preserved and reported. Writes go through a neighboring temporary file and atomic commit. An existing file is rechecked before commit to catch intervening edits; this is not a cross-process lock with RetroArch. Creation cannot overwrite a file created concurrently. A maintenance failure is nonfatal to the completed PKG installation or the launch of an already installed game.

## Validation Scope

`.ci/test-installed-playlist.cpp` uses synthetic playlist data only and covers creation, ownership collisions, appending, metadata and scalar-type preservation, Unicode/control escaping, duplicate/path handling, URI paths, core relocation, invalid input and the option schema. The existing option, PKG-routing and content-key tests provide separate regression coverage. No real PKG or live user playlist is required by these native checks.

Real installation, menu-refresh and direct game-launch behavior remain pending while runtime game tests are paused. Build and fixture results do not establish gameplay, license availability or frontend cache behavior.
