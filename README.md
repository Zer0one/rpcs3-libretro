# RPCS3 libretro

[RPCS3](https://rpcs3.net/) (PlayStation 3 emulator) as a libretro core for RetroArch and other libretro frontends.

This repository builds the core and nothing else. The standalone Qt application, its packaging and the input and audio backends a frontend replaces are not part of it; the emulator itself tracks upstream RPCS3.

The [integration policy](docs/INTEGRATION_POLICY.md) treats the original Libretro port as the reference base, with this fork's improvements adapted on top. It distinguishes that reference from the standalone RPCS3 engine and documents how to keep local differences limited and justified.

## Getting the core

**Latest numbered release: [v0.1.0.4](https://github.com/Zer0one/rpcs3-libretro/releases/tag/v0.1.0.4)** — RPCS3 v0.0.43-20415, runtime settings that preserve initialization-time fallbacks, enabled-device GPU limits and corrected Vulkan pause handoff. The installed-PSN playlist, section-local advanced controls and default-value labels remain available. See the [changelog](CHANGELOG.md#v0104--2026-10-29) for details.

This independent fork publishes numbered packages on its [GitHub Releases page](https://github.com/Zer0one/rpcs3-libretro/releases). Tag pushes start [Build And Publish Release](https://github.com/Zer0one/rpcs3-libretro/actions/workflows/release.yml): Linux x86_64/ARM64, Windows x86_64 and macOS Apple Silicon/Intel are built remotely and verified before automatic publication. Android publication is paused for releases after v0.1.0.2 until further notice; existing v0.1.0.1/v0.1.0.2 releases retain their seven-platform policy. Each ZIP contains the canonical core, matching `.info`, source/build identity and checksums. Copy the core and any adjacent runtime dependency into RetroArch's `cores` directory, and the `.info` into its `info` directory. Check the package metadata for minimum operating-system versions; the original local Apple Silicon package was built for macOS 26+.

The macOS core uses RetroArch's existing MoltenVK framework; no separate MoltenVK library is included. Sync is the default; Async requires a compatible driver and remains experimental. The [release procedure](docs/RELEASING.md) documents tag numbering, manual recovery for an existing tag and publication gates.

Core Option updates follow standalone RPCS3's dynamic-setting schema: settings marked dynamic can change during play; the others require restarting RetroArch. Requested values remain saved in the frontend's options, while the running session keeps its initialized values and hardware fallbacks. Per-title database overrides obey the same rule during play.

MSAA Auto and precise ZCULL are limited by the features enabled on the Vulkan device. Unsupported multisampled storage images or precise occlusion queries remain disabled after game loading and option updates. Sample shading, alpha-to-one, depth bounds, anisotropic filtering and wide-line checks use enabled device features. See [the configuration audit](docs/GPU_CONFIGURATION_AUDIT.md) for coverage and validation limits.

The original [WizzardSK core](https://github.com/WizzardSK/rpcs3-libretro) is distributed through RetroArch's Online Updater for Linux (x86_64 and aarch64), Windows x64, macOS (Apple Silicon and Intel) and Android (arm64 and x86_64). Those buildbot packages do not identify this fork's numbered release. The inherited [Build libretro core](https://github.com/Zer0one/rpcs3-libretro/actions/workflows/libretro.yml) workflow can produce additional test artifacts on request. Gameplay validation recorded in this repository covers Apple Silicon macOS; remote builds do not establish gameplay on other systems.

The PS3 firmware (`PS3UPDAT.PUP`, from playstation.com) goes into RetroArch's system directory as `system/rpcs3/PS3UPDAT.PUP`; the core installs it on first start. The firmware filename is recognized regardless of letter case.

### Licenses And Disc Keys

Licenses and disc keys are read in place, without automatic imports, copies or overwrites:

- **PSN RAP licenses:** opening a PKG makes its directory the first search location for the exact `<Content ID>.rap` requested by the game or installer. The next location is `system/rpcs3/data/rap/`. Existing licenses under the configured virtual HDD's `home/<user>/exdata/` remain a compatibility fallback. Mixed-case RAP extensions remain accepted in the PKG directory and central collection. Opening an installed `PARAM.SFO` or `EBOOT.BIN` directly has no source PKG directory, so use the central collection or an existing `exdata` license. Reset retains the PKG directory; closing/loading different content clears it.
- **Encrypted ISO disc keys:** look for `<ISO basename>.dkey`, then `<ISO basename>.key`, first beside the ISO, then in `system/rpcs3/data/key/`, `system/rpcs3/data/dkey/`, and finally the legacy `system/rpcs3/data/redump/`. Both extensions are accepted in both new collections. The existing reader supports a 16-byte binary key or a 32-character hexadecimal key. Keep the key's basename identical to the ISO's. Keys are checked against the image; an existing higher-priority invalid key is reported rather than silently replaced by a lower-priority one.

The core creates `data/rap/`, `data/key/` and `data/dkey/` when initialized. These collections are independent of the emulated HDD and PS3 user. Moving a PKG or ISO does not copy its sidecar files; a matching central file remains available. No licenses or keys are downloaded.

## Loading games

- A game in folder form is loaded by its `PS3_DISC.SFB` (a dumped disc) or `PARAM.SFO` (an installed or PSN game). Loading its `EBOOT.BIN` works too, but every game's executable has that name, so RetroArch's per-game and per-folder options can't tell the games apart; per-game option overrides only work when the game is loaded by its `.SFB` or `.SFO`.
- A base PS3 PSN game's `.pkg` can be kept as its RetroArch playlist entry: with **Core Options → System → PKG Launcher** enabled (default), the first load installs it and closes the content; opening the same PKG again starts the recognized installation without reinstalling. Save this option before reloading content; disabling it restores installation-only behavior for every PKG. The link uses package and installed-game identities, so renaming or moving the PKG still works, and the active `vfs.yml` HDD location is respected. An existing installation is recognized by its matching `PARAM.SFO` and `USRDIR/EBOOT.BIN`; a conflicting title/content identity is rejected. You can also start the installed game's `PARAM.SFO` in `dev_hdd0/game/<title ID>/` directly. Updates, DLC and other PKGs remain installation-only and are used by the corresponding game. RAP files remain at their original location and follow the search order above.
- RPCS3's log is `system/rpcs3/rpcs3_detailed.log`; a crash report goes to RetroArch's save directory as `rpcs3_libretro_crash.log`.

### Installed PSN Playlist

**Core Options → System → Installed PSN Playlist** is enabled by default. After a base PS3 PSN game PKG installs successfully, or when reopening a PKG that resolves to its existing installation, the core creates or updates **Sony - PlayStation 3 (PSN Installed)** in the frontend's configured playlist directory. Entries point to the installed game's `PARAM.SFO` and use the source PKG's filename without its final `.pkg` extension as the displayed label, preserving case, punctuation and region tags. The normal add/update operation refreshes that matching label on every recognized PKG opening and associates the currently loaded RPCS3 core. They work independently of PKG Launcher and the source PKG's location; direct installed-game launch needs a RAP in the central collection or existing HDD `exdata`, as described above. Updates, DLC, failed/incomplete installations and direct game launches do not add entries. Existing installations are not scanned automatically: reopen each base-game PKG to add that title. With PKG Launcher enabled, this does not reinstall the game.

The core adds an ownership marker to its playlist. An existing same-name playlist without the matching marker is left untouched, with an on-screen notification after installation. Deleting an unmarked same-name list and reopening a recognized base-game PKG lets the core create its own marked list and add that title without reinstalling. Existing titles are not duplicated; the matching label follows the opened PKG filename while other entries, item metadata and playlist settings are preserved. Disabling the option leaves playlists untouched. Unsupported, malformed or unreadable playlists are also preserved; a playlist failure never undoes a successful PKG installation. The frontend may require returning to its playlist list or restarting to display a newly created list.

See the [format, ownership and validation notes](docs/INSTALLED_PSN_PLAYLIST.md). Artwork remains a separate deferred idea.

## Tips

- **Database Settings Override** (on by default) applies RPCS3's per-game settings database, which covers most games' known issues. For a game that still misbehaves, its page in RPCS3's [compatibility list](https://rpcs3.net/compatibility) says what else it needs; the [Vblank compatible games list](https://wiki.rpcs3.net/index.php?title=Vblank_compatible_games_list) shows which games can run above 60 FPS.
- **Patch Manager** is always present. With no title loaded it shows “Load a title to check for patches”; with no matching listed patches it shows “No patches available for this title”, as a single-value informational row inside the section. Otherwise it lists real patch controls for the loaded game. It keeps its own settings: standalone RPCS3's `patch_config.yml` is not imported.
- Standalone RPCS3 scales its picture to the window with bilinear filtering; RetroArch scales with nearest neighbour unless told otherwise, which looks sharper and blockier. For the standalone look, turn on Settings > Video > Scaling > Bilinear Filtering (as a core override, so it applies to RPCS3 only), or prepend a `bilinear.slangp` shader.
- Core Options use one group per function. **Show Advanced Options** appears immediately before the controls it reveals inside **CPU** and **GPU** and independently reveals their advanced settings. At the main-menu level, **Show Advanced Sections** follows all normal sections, including Patch Manager; it defaults to Disabled and reveals **Scheduling (Advanced)**, **Timing (Advanced)** and **Diagnostics (Advanced)** at the end. Those three sections contain no internal visibility selector. Existing option classification, saved values and effects are preserved when controls/sections are hidden. This works with categorized and flat menus and is independent of RetroArch's global advanced-settings preference. Reopen Core Options if the frontend does not refresh immediately. Every adjustable option identifies its default with **(Default)** on the value label, for example **Approximate (Default)** for SPU XFloat Accuracy. Older API 0 frontends show **[Default: ...]** in the option name because they cannot separate value labels from saved values. See the [default-label convention](docs/CORE_OPTION_DEFAULTS.md), [research and visibility behavior](docs/ADVANCED_OPTIONS.md) and [full option inventory/standalone comparison](docs/CORE_OPTIONS_REVIEW.md).
- The PS3 user name is in `dev_hdd0/home/00000001/localusername`; edit it to change it.
- `dev_hdd0` and the other emulated drives can be moved outside RetroArch's folders by editing `vfs.yml` (in `system/rpcs3/`, or `system/rpcs3/config/` on Windows).
- `dev_hdd1` fills up with games' cache over time. Options > System > Disk Cache Size limits it: to 1 GB by default, or 5 GB (the caches used longest ago go first), wiped whenever a game is closed, or Unlimited. Its contents can also be deleted by hand at any time.
- A game that has built up a large SPU/shader cache can take a minute or more to close. Let it finish.
- **Surround sound:** set Options > Audio > Audio Format to Surround 5.1 or 7.1 to match your speakers (Stereo is the default). It is what the emulated PS3 tells games it is connected to, so a game with a surround mix plays it in 5.1/7.1. It needs a RetroArch with multi-channel audio output (master builds since September 2026), RetroArch's Settings > Audio > Output > Output Layout set to the same layout, and an audio device configured for it (on Windows, the speaker setup in the sound settings). With Stereo, games make their own stereo mix.
- PS3 peripherals beyond the DualShock 3 (guitars, PlayStation Move, cameras and the like) are not supported yet.

## Building

The Linux build runs in a container with the toolchain RPCS3's CI uses, through `.ci/build-libretro.sh`; see `.github/workflows/libretro.yml` for the exact invocation, and `.ci/build-windows-clang-libretro.sh` for Windows (MSYS2 clang64). A local build is a regular CMake build of the `rpcs3_libretro` target:

```bash
git clone --recursive https://github.com/Zer0one/rpcs3-libretro.git
cd rpcs3-libretro
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_SYSTEM_CURL=ON -DUSE_LIBEVDEV=OFF
cmake --build build --target rpcs3_libretro
```

LLVM is needed for the PPU/SPU recompilers (`-DLLVM_DIR=...`, or `-DSTATIC_LINK_LLVM=OFF` with a shared `libLLVM` next to the core).

### Local Apple Silicon Build

To use RetroArch's installed MoltenVK framework instead of a second provider:

```sh
RPCS3_RETROARCH_APP="/path/to/RetroArch.app" sh .ci/build-macos-frontend-libretro.sh
```

This uses Homebrew LLVM 22 and the local macOS version as its deployment target. The signed core, canonical `.info`, and LLVM's compression dependency are staged under `build-macos-arm64/dist/`. Install the core and compression library together in RetroArch's cores directory, and the `.info` in its info directory. MoltenVK stays in the frontend's application bundle. The runtime log verifies that core and frontend use the same Vulkan provider.

On macOS, enable **GPU → Show Advanced Options** above that section's advanced controls, then **Quick Menu → Core Options → GPU → MoltenVK Queue Submission** selects **Sync** (default) or **Async** for the core's Vulkan instance. Save the option and restart RetroArch to apply it; resetting the game can retain the existing Vulkan instance. This setting is independent of asynchronous shader compilation. Vulkan negotiation v2 passes the selection through `VK_EXT_layer_settings`, without changing the driver's global defaults. The startup log reports the actual submission mode, or warns when the driver lacks that extension and keeps its own default.

Sync remains the validated baseline. Async requires a compatible MoltenVK driver and remains experimental: framebuffer retirement fixes the reproduced startup lifetime fault, and a private acquisition-fence correction restored regular frame delivery in earlier Daytona tests, but the 3D/pause-resume acceptance check and the observed texture anomaly remain open for the final driver. Updating the frontend's MoltenVK alone does not change this core option. See the [Async investigation and evidence limits](MACOS_PERFORMANCE.md#async-framebuffer-lifetime-investigation).

When available, RPCS3 and RetroArch use separate Vulkan queues on their shared device. New frames carry a GPU semaphore dependency, and images remain held until every frontend sync index that sampled them has completed, including repeated frames. Only frontend-facing images use concurrent sharing when the queue families differ. A GPU without an available separate queue retains the shared-queue path. Queue selection is automatic and preserves the selected VSync and MoltenVK submission settings. The runtime log identifies the renderer, frontend and transfer families. See the [Daytona result and validation limits](MACOS_PERFORMANCE.md#separate-queue-correction).

For performance diagnosis, enable **Show Advanced Sections** after the normal sections, then open **Quick Menu → Core Options → Diagnostics (Advanced) → Performance Timing Log**. It writes `[RPCS3 Timing]` summaries once per second to RetroArch's log and `system/rpcs3/rpcs3_detailed.log`. It is disabled by default and can be changed while content runs. The counters separate frontend calls, new/duplicate frames, audio/video callbacks and Vulkan hardware queue/image/fence waits. They measure CPU wall time, not GPU execution time or physical scanout. The [timing-log guide and measured presentation bottleneck](MACOS_PERFORMANCE.md#synchronous-timing-log-investigation) explain the fields. Summarize the uncompressed RPCS3 log with `python3 .ci/summarize-libretro-timing.py /path/to/rpcs3_detailed.log`.

The [local macOS validation report](MACOS_VALIDATION.md) records the successful Daytona boot and race, the initial-cache failure, and the remaining native Cmd-Q teardown limitation. For that build, use RetroArch's internal close/quit path. If Daytona faults in `_cellsurMixerMain` on its first start, preserve its cache and retry; do not treat a loading-screen FPS counter as a successful boot.

## Roadmap

See the [roadmap](ROADMAP.md) for optional ideas and their implementation limits, including post-install artwork for existing user-created playlists.

## License

Most files are licensed under the terms of the GNU GPL-2.0-only license; see the LICENSE file for details. Some files may be licensed differently; check the appropriate file headers.
