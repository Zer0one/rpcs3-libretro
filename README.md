# RPCS3 libretro

[RPCS3](https://rpcs3.net/) (PlayStation 3 emulator) as a libretro core for RetroArch and other libretro frontends.

This repository builds the core and nothing else. The standalone Qt application, its packaging and the input and audio backends a frontend replaces are not part of it; the emulator itself tracks upstream RPCS3.

## Getting the core

This independent fork publishes numbered packages on its [GitHub Releases page](https://github.com/Zer0one/rpcs3-libretro/releases). Tag pushes start [Build And Publish Release](https://github.com/Zer0one/rpcs3-libretro/actions/workflows/release.yml): Linux x86_64/ARM64, Windows x86_64, macOS Apple Silicon/Intel and Android ARM64/x86_64 are built remotely and verified before automatic publication. Android remains experimental. Each ZIP contains the canonical core, matching `.info`, source/build identity and checksums. Copy the core and any adjacent runtime dependency into RetroArch's `cores` directory, and the `.info` into its `info` directory. Check the package metadata for minimum operating-system versions; the original local Apple Silicon package was built for macOS 26+.

The macOS core uses RetroArch's existing MoltenVK framework; no separate MoltenVK library is included. Sync is the default; Async requires a compatible driver and remains experimental. The [release procedure](docs/RELEASING.md) documents tag numbering, manual recovery for an existing tag and publication gates.

The original [WizzardSK core](https://github.com/WizzardSK/rpcs3-libretro) is distributed through RetroArch's Online Updater for Linux (x86_64 and aarch64), Windows x64, macOS (Apple Silicon and Intel) and Android (arm64 and x86_64). Those buildbot packages do not identify this fork's numbered release. The inherited [Build libretro core](https://github.com/Zer0one/rpcs3-libretro/actions/workflows/libretro.yml) workflow can produce additional test artifacts on request. Gameplay validation recorded in this repository covers Apple Silicon macOS; remote builds do not establish gameplay on other systems.

The PS3 firmware (`PS3UPDAT.PUP`, from playstation.com) goes into RetroArch's system directory as `system/rpcs3/PS3UPDAT.PUP`; the core installs it on first start. Disc keys for redump ISOs go into `system/rpcs3/data/redump/`, named like the ISO with a `.key` or `.dkey` extension.

## Loading games

- A game in folder form is loaded by its `PS3_DISC.SFB` (a dumped disc) or `PARAM.SFO` (an installed or PSN game). Loading its `EBOOT.BIN` works too, but every game's executable has that name, so RetroArch's per-game and per-folder options can't tell the games apart; per-game option overrides only work when the game is loaded by its `.SFB` or `.SFO`.
- Loading a `.pkg` installs it to `dev_hdd0` (one at a time); a game is then started from its `PARAM.SFO` in `dev_hdd0/game/<title ID>/`, and its installed updates and DLC are used by it, a disc game's too. The `.rap` licenses next to the `.pkg` are installed with it; RPCS3 finds them by name, so each has to be named after its content ID (`<content ID>.rap`), as PSN dumps come.
- RPCS3's log is `system/rpcs3/rpcs3_detailed.log`; a crash report goes to RetroArch's save directory as `rpcs3_libretro_crash.log`.

## Tips

- **Database Settings Override** (on by default) applies RPCS3's per-game settings database, which covers most games' known issues. For a game that still misbehaves, its page in RPCS3's [compatibility list](https://rpcs3.net/compatibility) says what else it needs; the [Vblank compatible games list](https://wiki.rpcs3.net/index.php?title=Vblank_compatible_games_list) shows which games can run above 60 FPS.
- The **Patch Manager** category lists the loaded game's patches and turns them on per game. It keeps its own settings: standalone RPCS3's `patch_config.yml` is not imported.
- Standalone RPCS3 scales its picture to the window with bilinear filtering; RetroArch scales with nearest neighbour unless told otherwise, which looks sharper and blockier. For the standalone look, turn on Settings > Video > Scaling > Bilinear Filtering (as a core override, so it applies to RPCS3 only), or prepend a `bilinear.slangp` shader.
- A handful of rarely needed options are not in the menu, as standalone RPCS3 keeps them out of its settings dialog, but can be set in the core's `.opt` file (`RPCS3.opt`).
- The PS3 user name is in `dev_hdd0/home/00000001/localusername`; edit it to change it.
- `dev_hdd0` and the other emulated drives can be moved outside RetroArch's folders by editing `vfs.yml` (in `system/rpcs3/`, or `system/rpcs3/config/` on Windows).
- `dev_hdd1` fills up with games' cache over time. Options > Disk Cache Size limits it: to 1 GB by default, or 5 GB (the caches used longest ago go first), wiped whenever a game is closed, or Unlimited. Its contents can also be deleted by hand at any time.
- A game that has built up a large SPU/shader cache can take a minute or more to close. Let it finish.
- **Surround sound:** set Options > Audio Format to Surround 5.1 or 7.1 to match your speakers (Stereo is the default). It is what the emulated PS3 tells games it is connected to, so a game with a surround mix plays it in 5.1/7.1. It needs a RetroArch with multi-channel audio output (master builds since September 2026), RetroArch's Settings > Audio > Output > Output Layout set to the same layout, and an audio device configured for it (on Windows, the speaker setup in the sound settings). With Stereo, games make their own stereo mix.
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

On macOS, **Quick Menu → Core Options → GPU → MoltenVK Queue Submission** selects **Sync** (default) or **Async** for the core's Vulkan instance. Save the option and restart RetroArch to apply it; resetting the game can retain the existing Vulkan instance. This setting is independent of asynchronous shader compilation. Vulkan negotiation v2 passes the selection through `VK_EXT_layer_settings`, without changing the driver's global defaults. The startup log reports the actual submission mode, or warns when the driver lacks that extension and keeps its own default.

Sync remains the validated baseline. Async requires a compatible MoltenVK driver and remains experimental: framebuffer retirement fixes the reproduced startup lifetime fault, and a private acquisition-fence correction restored regular frame delivery in earlier Daytona tests, but the 3D/pause-resume acceptance check and the observed texture anomaly remain open for the final driver. Updating the frontend's MoltenVK alone does not change this core option. See the [Async investigation and evidence limits](MACOS_PERFORMANCE.md#async-framebuffer-lifetime-investigation).

When available, RPCS3 and RetroArch use separate Vulkan queues on their shared device. New frames carry a GPU semaphore dependency, and images remain held until every frontend sync index that sampled them has completed, including repeated frames. Only frontend-facing images use concurrent sharing when the queue families differ. A GPU without an available separate queue retains the shared-queue path. Queue selection is automatic and preserves the selected VSync and MoltenVK submission settings. The runtime log identifies the renderer, frontend and transfer families. See the [Daytona result and validation limits](MACOS_PERFORMANCE.md#separate-queue-correction).

For performance diagnosis, enable **Quick Menu → Core Options → Debug → Performance Timing Log**. It writes `[RPCS3 Timing]` summaries once per second to RetroArch's log and `system/rpcs3/rpcs3_detailed.log`. It is disabled by default and can be changed while content runs. The counters separate frontend calls, new/duplicate frames, audio/video callbacks and Vulkan hardware queue/image/fence waits. They measure CPU wall time, not GPU execution time or physical scanout. The [timing-log guide and measured presentation bottleneck](MACOS_PERFORMANCE.md#synchronous-timing-log-investigation) explain the fields. Summarize the uncompressed RPCS3 log with `python3 .ci/summarize-libretro-timing.py /path/to/rpcs3_detailed.log`.

The [local macOS validation report](MACOS_VALIDATION.md) records the successful Daytona boot and race, the initial-cache failure, and the remaining native Cmd-Q teardown limitation. For that build, use RetroArch's internal close/quit path. If Daytona faults in `_cellsurMixerMain` on its first start, preserve its cache and retry; do not treat a loading-screen FPS counter as a successful boot.

## License

Most files are licensed under the terms of the GNU GPL-2.0-only license; see the LICENSE file for details. Some files may be licensed differently; check the appropriate file headers.
