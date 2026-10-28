# Roadmap

Optional ideas are not committed implementation work. Status distinguishes a proposed feature from an implemented or runtime-validated one.

Legend: 🟢 Completed · 🟡 Partial · ⚪ Not Started · 🔵 Deferred

| Status | Activity | Remaining Work |
| --- | --- | --- |
| 🟡 | Installed PSN Playlist | Implemented as an optional update after installation or reopening a recognized PKG, maintaining a dedicated, marked playlist pointing to installed PARAM.SFO files. Synthetic format/ownership tests and the native macOS core build passed; real PKG-install/frontend validation remains subject to the runtime testing pause. See [the design and validation notes](docs/INSTALLED_PSN_PLAYLIST.md). |
| 🔵 | Optional Post-Install Playlist Thumbnails | After a successful PKG installation, optionally copy the installed game's `ICON0.PNG` to RetroArch's thumbnail directory for a matching entry in an existing playlist. Read the entry's label, database association and thumbnail matching mode; preserve existing artwork. The artwork feature must not create playlists or modify their entries; the separately authorized Installed PSN Playlist feature owns its own list. Resolve the frontend's actual thumbnail directory without assuming a fixed path: the current Libretro API exposes the playlist directory but not the thumbnail directory. Design and runtime validation remain deferred. |
