# Apple Silicon Validation — October 25, 2026

## Build And Driver

- Host: Apple M4, macOS 26.5.1, arm64.
- Engine: official RPCS3 `v0.0.43-20400`, `f848fd1e12aaba4f7c04bbd8642bfb9b5fae556c`.
- Local full upstream merge: `b5e49b227`; the macOS integration changes are additional uncommitted work. No refs were published.
- Local compiler: Homebrew LLVM 22.1.8, Release, macOS deployment target 26.
- The local build script links RetroArch's existing `@rpath/MoltenVK` framework. It packages zstd beside the core, without a Homebrew runtime dependency or a second MoltenVK copy.
- Runtime provider checks and `lsof` confirmed the framework inside the active RetroArch app. Its SHA-256 was `6c511b765b68df9043b05fe6cdaacb78ce256115aab887a2b9d9c582143aa22b`; the driver was not changed.
- The initial gameplay/comparison core SHA-256 is `fea5912ecf7ff5ed8e7d1e16839de5ebf8f207cd1c2eb636ffd86a83d022eb56`. Its matching info file reports build 20400. Ad-hoc signing verification passed. The subsequent [timing-log build and deployment](MACOS_PERFORMANCE.md#synchronous-timing-log-investigation) add opt-in diagnostics while retaining synchronous submission.

## Standalone Reference

The official unmodified Apple Silicon app for the same engine revision was extracted in the separate standalone workspace, under `/Users/andrea/dev/rpcs3/.local/comparison-20400/`.

The successful standard run used an empty configuration (`{}`), the app's bundled drivers, firmware 4.92 and the existing Daytona USA Europe installation (`NPEB00630`). The log identifies the selected device as Apple M4 (MoltenVK). No Xenia driver or asynchronous-submission override was injected into this standard run.

The first cold-cache standard run failed at `_cellsurMixerMain`, executing address zero, after a failed CRI SPU group destruction (`CELL_EBUSY`). A second run with unchanged settings and the persistent cache passed that point. Keyboard input dismissed the initial save warnings, created a save in the private profile and reached the rendered 3D introduction. This establishes a positive boot and input response, not a full standalone gameplay benchmark.

[RPCS3 issue #18852](https://github.com/RPCS3/rpcs3/issues/18852) reports a related CRI/surmixer cold-cache failure and a warm-cache workaround. [PR #18857](https://github.com/RPCS3/rpcs3/pull/18857) already belongs to build 20400; no duplicate fix was applied. Similar symptoms do not by themselves establish the same root cause.

Other installed games were copied into the isolated profile, but were not launched. Once Daytona provided the positive reference, further game and KosmicKrisp launches were omitted at the user's request.

## Core Results

| Configuration | Observed Result |
| --- | --- |
| LLVM PPU/SPU, initial cache, RetroArch MoltenVK, Async | Guest `_cellsurMixerMain` fault at address zero; no gameplay |
| Static PPU interpreter, RetroArch MoltenVK, Async | Host fault inside `MVKRenderSubpass::populateMTLRenderPassDescriptor`; interpreter did not provide a working alternative |
| LLVM PPU/SPU, persistent cache, RetroArch MoltenVK, synchronous submissions | Title, Main Menu, Arcade settings, Beginner selection, race start and accelerator response verified |
| Same final build, normal frontend teardown | Emulation Join Thread cleared objects; RetroArch unloaded game/core/symbols and exited; no new crash report observed |

The successful race used the database override (Disable Vertex Cache and PS3 Native frame limit), 100% resolution, and the runner's keyboard bindings. An interactive observation showed lap 1/8 and 135 km/h after the accelerator input. The retained `gameplay.png` is a later observation at 0 km/h, with RPCS3's overlay at 43.59 FPS and RetroArch's at 60.00 FPS. These are instantaneous overlay readings, not a measured average or a matched standalone comparison. The test was bounded; audio quality, physical gamepads, extended stability, reset and a complete race were not validated.

The Xenia marker is detected in the exact frontend provider. The final build requests **synchronous** submission for its own Vulkan instance through `VK_EXT_layer_settings`; it does not change global MoltenVK defaults. The initial controls differed in decoder/cache state and could not isolate asynchronous submission. A subsequent [matched LLVM control and attract-mode comparison](MACOS_PERFORMANCE.md) reproduces the Async host fault with the existing cache and captures its MoltenVK image-view/render-pass stack. No Async benefit is claimed.

Native macOS Cmd-Q took an AppKit process-exit path and faulted while static Vulkan overlay resources were being destroyed after their allocator. That path is not fixed here. The subsequent internal frontend shutdown test completed the normal game/core cleanup. Use RetroArch's internal close/quit path when testing this build.

## Isolation And Evidence

Core runs use `build-macos-arm64/integration-2026-10-25/ra-20400/`, with separate firmware, content copies, cache, options, saves, logs and command port. The final test's `frontend.log`, `runtime.json`, `system/rpcs3/rpcs3_detailed.log`, and `gameplay.png` preserve the evidence. Earlier controls are in `llvm-result/`, `interpreter-async-result/` and `sync-warm-result/`. Build logs and the preserved old core are in the parent integration directory.

Standalone evidence is in its separate workspace: `standard-results/daytona-default-cold/` and `standard-results/daytona-default-warm/`. The original RPCS3 application-support and cache directories were temporarily redirected and restored after each valid reference run; the manifests record restoration. The generated compressed log had mode 000, so the readable uncompressed log was archived instead. The runner was corrected to restore original directories before optional log archival.

An earlier invalid control had redirected only the host cache directory. The app consequently read the user's original VFS configuration, firmware 4.90 and emulated drives, and automatically cleared 1181.35 MB of regenerable emulated disk cache. That unintended cleanup was disclosed to the user. This control is excluded from the firmware-matched comparison; its logs are retained under `cache-fw490-invalid-control/`. The later standard runs verified both VFS destinations and firmware 4.92.

The final local deployment uses `rpcs3_libretro.dylib`, its matching `.info`, and `libzstd_macos_arm64.dylib`. The deployment receipt records source/destination hashes and the backed-up installed files. RetroArch's global configuration, app bundle and MoltenVK are not deployment targets.

Further tests must read content directly from its existing archive or installation path where supported. A new content copy requires the user's explicit authorization. The copies prepared before that directive remain in place; none were deleted or moved in response to it.

## Synchronous Queue Correction — October 25, 2026

The separate Vulkan queue implementation and its evidence are documented in [the performance report](MACOS_PERFORMANCE.md#separate-queue-correction). Daytona's measured completed-image rate improves from 52.73 to 59.86 Hz with VSync and synchronous MoltenVK enabled. Checks cover normal close, pause/resume, reset back to 3D, two/three frontend images and a forced shared-queue fallback. Khronos synchronization validation reports no hazards; four API-validation categories also occur in the previous-core control and remain unresolved. This is not a fully clean validation result or an Async fix.

The separate-queue activity installed core SHA-256 `0ce0ccdcb1e20e12b376e7cecfa5af9fd40abb60488779ba75916ed0ec3e31d1`; backup and deployment receipt are under `build-macos-arm64/queues-2026-10-25/deployment/`. The installed core/info and `build-macos-arm64/dist/` agree. Global RetroArch configuration and the existing MoltenVK framework are unchanged; all test processes exited normally.


## Async Lifetime Follow-Up

The [framebuffer lifetime investigation](MACOS_PERFORMANCE.md#async-framebuffer-lifetime-investigation) supersedes the unresolved-crash status above. The official build-20400 standalone reproduced the Async fault in its shared framebuffer-cache path; bundled KosmicKrisp separately reached Daytona 3D and closed normally. OpenGL was not tested.

The core now retires unused framebuffers through RPCS3's existing GPU event resource manager. A private Async build passed native UI, 3D, pause/resume, reset back to 3D and normal close with API/synchronization validation. The framebuffer `00892` error disappeared, no synchronization hazard was reported, and the three existing feature-validation categories remained. Async frontend delivery still showed frequent duplicates/drops despite approximately 60 completed images/s, so the final core retains synchronous submission. These results cover Daytona on this Apple M4, not every game or driver.


The final synchronous core `4bce48f3bd90304a831b9b9326678b4494f4a1177d5543c0906825c08cc11531` passed a separate 3D/normal-close check, with 59.903 new images/s and zero drops in the selected 49.480-second interval. It is installed under the canonical core name with matching `.info`; `build-macos-arm64/async-2026-10-25/deployment/receipt.json` records hashes, backup and strict signature verification. The global RetroArch configuration and driver are unchanged.


## Xemu Acquisition Fence Control

The [same-binary MoltenVK experiment](MACOS_PERFORMANCE.md#xemu-acquisition-fence-comparison) restores Async frontend delivery from 33.56 to 59.88 new images/s with essentially unchanged renderer throughput. Dropped images fall from 1,310 to zero; only the private acquisition-fence completion switch changes between performance runs.

Separate two-swapchain-image validation passes boot/3D, pause/resume, reset and normal close with the correction enabled, and a pause/resume/close control without it. Both retain only the three known feature-validation categories, with no synchronization hazard or framebuffer lifetime report. A suspect texture anomaly captured after resume in the enabled run is unresolved; later post-reset output is correct, and unmatched off-control captures do not establish causality. The private driver is not installed globally. The canonical synchronous core remains unchanged and is copied again to local RetroArch with a fresh deployment receipt under `build-macos-arm64/acquire-fence-2026-10-25/deployment/`.


## MoltenVK Queue Submission Option — 25 October 2026

The macOS GPU option now exposes Sync (default) and experimental Async, applied at Vulkan instance creation. Save the selection and restart RetroArch. ARM64 compilation, signing and no-GPU option/instance-setting checks passed (API 0/1/2, English/Italian fallback, missing options/extension and provider rejection). No game or GPU session was run for this change; prior 3D/pause-resume limitations remain open. The canonical local core and matching info are installed, core SHA-256 `a7558b77c9e4026673ae5ee41abef77cfef56bc7c9f032ae4543242c5e632134`. Frontend configuration and final MoltenVK remain unchanged. Evidence and backup receipt: `build-macos-arm64/moltenvk-option-2026-10-25/REPORT.md` and `deployment/receipt.json`.
