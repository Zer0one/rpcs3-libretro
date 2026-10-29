# Runtime Configuration And GPU Capability Audit

The reference is standalone RPCS3 at upstream build 20410. The Libretro adapter must preserve its configuration semantics unless a specific upstream bug is established. This audit covers the local changes based on core commit `3e579ff2f095cf7d60638ff21eb7be79a9a6b5d6`; that commit identifier alone does not identify the modified test binary.

## Requested And Effective Settings

Standalone `Emulator::SaveSettings` passes `!Emu.IsStopped()` to `g_cfg.from_string`. While emulation runs, `cfg::decode` accepts only entries marked dynamic in `Emu/system_config.h`. Restart-only CPU/GPU settings therefore cannot contradict already initialized resources.

The former Libretro refresh path reapplied all Core Options directly to `g_cfg`, then reapplied database settings. This could undo initialization-time fallbacks even when the user changed an unrelated option. The adapter now prepares the requested configuration separately and applies it through the same upstream dynamic filter. Database overrides retain their precedence and pass through that filter too. This is a schema-wide fix, rather than a list of special cases for MotorStorm.

RetroArch retains requested option values in its own options file. For example, MSAA can remain Auto in the menu while the renderer uses Disabled because the device lacks required features. Shader Mode changes are saved for a new session; Resolution Scale and volume can change during play according to upstream's schema. No additional persistence policy is imposed on the standalone's runtime settings.

## Reviewed Paths

| Path | Finding And Treatment |
| --- | --- |
| MSAA | Frontend Vulkan negotiation can precede game configuration loading. Loading or refreshing settings could restore Auto after device initialization had disabled unsupported MSAA. Reapply the enabled-device limit before renderer resources are created and after runtime refresh. |
| Precise ZCULL | Do not request precise queries when `occlusionQueryPrecise` is absent from the enabled logical-device features. Preserve the saved request. |
| Sample Shading | Pipeline creation also checks enabled `sampleRateShading`, in addition to effective MSAA. |
| Alpha-To-One, Depth Bounds, Anisotropy, Wide Lines | Existing support queries now read features enabled on the logical device, rather than physical-device availability alone. |
| Async Texture Streaming | Backend queue availability already limits asynchronous uploads. The user setting is not dynamic and is now protected from runtime rewrites. |
| Vulkan Queue Scheduler | Initialization-time vendor fallbacks are protected by the upstream non-dynamic classification. This does not validate Async submission on MoltenVK. |
| CPU Decoder Fallbacks | The same dynamic filter preserves initialized CPU choices during option refresh; the linked-core fixture checks an SPU decoder example. |
| OpenGL Shader Interpreter | The missing-bindless-texture fallback changed `g_cfg` but left a cached local shader mode unchanged. Resource initialization now uses the fallback as well. This is a separate initialization defect, not evidence that every difference is specific to Libretro. |

The core and RetroArch share a Vulkan device negotiated before game boot. Standalone initializes its renderer through a different lifecycle. Reusing renderer code does not automatically preserve assumptions about when configuration is loaded or which features are enabled. Both device paths use the same capability guard; there is no title-specific exception.

## Validation

- The established macOS frontend build helper completed, including dependency and signature checks. This local build targets macOS 26; it does not validate release compatibility with macOS 15.
- `.ci/test-runtime-config.cpp` links the built core and exercises its real configuration schema: dynamic Resolution Scale/volume changes, unchanged initialized MSAA/shader/ZCULL/scheduler/SPU values, repeated refresh, and acceptance of requested values during non-dynamic loading. An empty logical-device feature fixture exercises the actual MSAA/ZCULL guard and selected support queries. It is not a physical-GPU compatibility test.
- The Core Options audit passed for 102 options, including 70 advanced options, the supported option API versions, defaults, localization and Patch Manager.
- MotorStorm BCES00006 rendered vehicle selection with the tested build. After changing Resolution Scale from 100 to 150 and requesting Interpreter Only, the log confirmed that effective MSAA remained Disabled and Shader Mode remained the initialized Async Recompiler with Shader Interpreter. The frontend saved the requested values on exit.
- That first refresh run also recorded a fatal `Dead FIFO commands queue state` after roughly 160 seconds paused in the menu. The subsequent screenshot was stale. It is **not** a successful post-refresh rendering test, and the cause is not established by this audit.
- A second isolated run used the same binary and option changes with RetroArch's menu pause disabled. Rendering continued after the refresh, and keyboard input changed the selected bike from Mohawk Indiana 1 to Monarch Champ 1. Telemetry continued to report approximately 30 new frames per second, with no fatal error before shutdown. The options file retained MSAA Auto, Resolution Scale 150 and Interpreter Only, while runtime logs retained effective MSAA Disabled and Async Recompiler with Shader Interpreter. This is a bounded vehicle-selection positive, not a race or a fix for the paused-menu failure.

The binary SHA256 is `60269f0d759ddee086b60dedeff5ec97642866bd368c868cf9cdc536131faa03`. Local evidence, source diff, build log, configuration fixture output and runtime logs are under `build-macos-arm64/gpu-config-audit-2026-10-28/`. MoltenVK submission was Sync, MSAA requested Auto, Shader Mode initially Async with Shader Interpreter, and FIFO accuracy Ordered & Atomic. Profiles and writable game data were isolated; the original ISO and global RetroArch configuration were preserved.

The exact tested binary was installed atomically as `rpcs3_libretro.dylib` in the local RetroArch cores directory, alongside its verified dependency, with the info file in the configured info directory. Previous files are in the evidence directory's `deployment-backup`; `deployment.json` records hashes. RetroArch is stopped. The unpaused-menu run required SIGTERM after CLOSE_CONTENT/QUIT did not finish within the timeout; a successful launcher exit code does not establish clean core shutdown. The earlier failed-rendering run exited without forced termination.

OpenGL is excluded from this macOS build, so its change is code-reviewed but neither compiled nor runtime-tested here. There is no hardware matrix, full-game stability claim, or same-version standalone attribution for the new FIFO failure. Earlier short race positives for the MSAA-only build are documented separately and do not substitute for validation of this build.

## Follow-Up On Engine 20415

Both core and standalone were updated to engine 20415. The standalone passed a 199.534-second emulator pause and approximately 80 seconds of subsequent rendering/input, with one transient guest Present timeout and no fatal. The installed core then reproduced Dead FIFO approximately 1.065 seconds after resuming a 230.949-second RetroArch menu pause, with **no option changes**. The earlier failure therefore cannot be attributed solely to the option refresh. Evidence is in `build-macos-arm64/motorstorm-pause-20415-2026-10-28/REPORT.md` and `result.json`.

The remaining differences include pause integration, pause duration and the driver: RetroArch's patched MoltenVK versus standalone's original bundled library. The result warrants investigation of those paths; it does not yet identify a root cause. No corrective source change was made for this follow-up.

## Vulkan Pause Handoff Correction

The subsequent investigation found that `VKLibretro::frame_ready` could hold RSX inside frame retirement for the entire frontend pause, even after `Emulator::Pause` had set the guest pause flags. The standalone RSX path instead reaches `cpu_thread::check_state` / `rsx::thread::cpu_wait`, which allows backend-local work during pause. The core's `Pause(false, false)` call itself uses upstream semantics; its second argument only suppresses the pause overlay. The failing run also used emulator pacing, not frontend VBlank pacing.

The handoff now stops waiting when `Emu.IsPaused()` becomes true and returns to upstream's pause path. It does not reset FIFO state, alter guest clocks or driver settings, or suppress the fatal detector. A process sample during the corrected run confirmed RSX waiting in `cpu_wait`, and the log recorded handoff release 2.315 ms after the pause message.

With engine 20415 and the same patched RetroArch MoltenVK in Sync mode, MotorStorm passed a 259.231-second menu pause. Rendering resumed and Right changed the selected bike from Mohawk Indiana 1 to Monarch Champ 1. The transient guest Present timeout also seen in standalone remained. Evidence and the exact candidate diff/hash are in `build-macos-arm64/motorstorm-pause-handoff-20415-2026-10-28/`; the final report records 92.243 seconds of post-resume visual observation and a separate shutdown timeout requiring SIGTERM. This is a bounded vehicle-selection result supporting the handoff correction, not proof that every FIFO failure has this cause. Async submission, Frame Advance precision, other renderers and full-race stability remain outside this test.

## Shutdown And Async Controls, October 29

A fresh Sync run with the same installed candidate (`d52b4a0648e3770617ab8d4d910648e338e4d95b3b9237bf27f1ada846fb0eea`) reached the rendered Mohawk Indiana 1 selection. After `CLOSE_CONTENT`, RetroArch returned visibly to its main menu with no core selected. `GET_STATUS` answered `CONTENTLESS` at both diagnostic checkpoints, 10 and 30 seconds after the runner's early `QUIT` sequence. RPCS3 logged object cleanup and ISO unloading; process samples showed the frontend rendering its menu, without an RPCS3 emulation-thread stack. The core library still appeared in the samples' binary-image lists, so this does not prove that its image was unmapped.

The UDP `QUIT` sequence, including a later single retry, did not exit the frontend. Native Command-Q then exited it without SIGTERM or SIGKILL. The isolated frontend configuration had `confirm_quit = true`; the precise reason the UDP sequence failed remains unproven. The earlier forced termination is therefore not sufficient evidence of a core shutdown deadlock. Future lifecycle tests must distinguish content closure from frontend exit, wait for an observed contentless state, and account for the frontend's quit confirmation before using a forced termination as evidence. This run did not retest a long pause or establish immediate image unloading.

The standalone control used the existing official native ARM64 RPCS3 0.0.43-20415 installation and its original MoltenVK 1.4.2, with asynchronous queue submission enabled. It crashed at emulator time 2.568718 seconds before vehicle selection. The first null-read fault symbolicated to `MVKImageView::populateMTLRenderPassAttachmentDescriptor + 28` on `MoltenVKQueue-0-0-0.0-Dispatch`. A subsequent allocator trap while reporting the error was a separate termination stage. This establishes a startup failure in the standalone renderer/driver path; it does not establish who invalidated the resource or that prior core Async failures have the same cause. No core Async run or Async shutdown/pause test followed this failed standalone control. Sync remains the baseline.

Core evidence is under `build-macos-arm64/motorstorm-lifecycle-2026-10-29/REPORT.md` and `sync-close/`; standalone evidence is under the sibling standalone checkout's `.local/motorstorm-async-shutdown-20415-2026-10-29/REPORT.md`. Both game processes are stopped. The core test preserved the installed core, frontend driver, global RetroArch configuration and original ISO metadata; it used isolated writable game data. No emulator-source change was made for these controls.

### Async Lifetime Investigation Follow-Up

The existing October 25 Daytona investigation in `MACOS_PERFORMANCE.md` is directly relevant: the core already defers unused framebuffer destruction through RPCS3's resource manager. Inspection of upstream commit `6c44fff7549e941389a4ce072a95c5ff9db7237c` and disassembly of the installed official 20415 executable confirm that standalone still destroys cached framebuffers and their owned image views directly from `vk::remove_unused_framebuffers`. Engine-version equality therefore does not imply equality of this already-patched lifetime path.

Two short standalone debugger runs used the same installed executable and original MoltenVK. The first recorded 480 image-view destruction calls, 474 from unused-framebuffer eviction, but its breakpoints perturbed timing and it stopped at a PPU JIT memory access instead of the target driver fault. This is not a rendering positive. The second removed the destruction breakpoints and reproduced the target null read in the Async dispatch thread: `x8 = 0`, with `ldr x21, [x8]` at `populateMTLRenderPassAttachmentDescriptor + 28`. The official MoltenVK 1.4.2 source identifies this as the unchecked `_planes[0]` access. The debugger killed the stopped process intentionally after collecting the fault; this is not a shutdown failure.

This matches the failure mechanism described by [MoltenVK issue 2810](https://github.com/KhronosGroup/MoltenVK/issues/2810) and makes the existing framebuffer-retirement correction the primary candidate to validate in standalone before adding a driver workaround. The two runs do not connect a specific destroyed handle to the faulting object in the same execution; no same-version standalone build with that correction was tested. The cause remains strongly supported rather than conclusively proved for MotorStorm. No new core/driver patch, build or deployment was made. Detailed evidence and limitations are recorded in the standalone checkout at `.local/motorstorm-async-lifetime-20415-2026-10-29/REPORT.md`.
