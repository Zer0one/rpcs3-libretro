# Roadmap

Optional ideas are not committed implementation work. Status distinguishes a proposed feature from an implemented or runtime-validated one.

Legend: 🟢 Completed · 🟡 Partial · ⚪ Not Started · 🔵 Deferred

| Status | Activity | Remaining Work |
| --- | --- | --- |
| 🔵 | Optional Post-Install Playlist Thumbnails | After a successful PKG installation, optionally copy the installed game's `ICON0.PNG` to RetroArch's thumbnail directory for a matching entry in an existing user-created playlist. Read the entry's label, database association and thumbnail matching mode; preserve existing artwork. Never create or modify playlists. Resolve the frontend's actual thumbnail directory without assuming a fixed path: the current Libretro API exposes the playlist directory but not the thumbnail directory. Design and runtime validation remain deferred. |
