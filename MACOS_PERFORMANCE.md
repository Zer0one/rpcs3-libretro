# Daytona Attract-Mode Comparison — October 25, 2026

The initial synchronous core ran Daytona's automatic 3D sequence below the same-build standalone. The [timing-log investigation](#synchronous-timing-log-investigation) traced a bottleneck to presentation holding the shared Vulkan queue lock. The [separate-queue correction](#separate-queue-correction) restores approximately 60 completed images per second with VSync and synchronous MoltenVK submission enabled. The [Async follow-up](#async-framebuffer-lifetime-investigation) fixes the reproduced framebuffer lifetime crash; the [Xemu acquisition-fence control](#xemu-acquisition-fence-comparison) restores regular Async delivery with a private MoltenVK candidate. The installed driver retains synchronous submission as the default.

## Results

These are samples of RPCS3's own overlay, whose readings average approximately 350 ms. They are not RetroArch's presentation FPS, a per-frame trace, or a whole-run average.

| Measurement | Official Standalone, Sync | Libretro Core, Sync |
| --- | ---: | ---: |
| 3D Samples | 9 | 11 |
| Median Sampled FPS | 59.96 | 54.23 |
| Lowest Sampled FPS | 59.86 | 34.20 |
| Highest Sampled FPS | 60.02 | 59.88 |
| Median Overlay Frame Interval | 16.7 ms | 18.4 ms |
| Highest Overlay Frame Interval | 16.7 ms | 29.2 ms |
| Title Samples, Excluded Above | 3 | 2 |
| Median Title FPS | 59.94 | 57.12 |
| Ranking Samples, Excluded Above | 2 | 1 |

The ratio of sampled 3D medians is 90.44%. The 34.20 FPS reading is one retained sample, not a sustained rate. Titles and ranking overlays are reported separately rather than improving the 3D statistic. The standalone's approximately 60 FPS ceiling does not reveal its unused performance headroom.

Host process CPU medians over each sampling window, including interludes, were 110.4% for standalone and 75.8% for RetroArch; 100% represents one logical CPU. RSS medians were approximately 1406 and 1422 MiB. These counters do not identify the limiting thread or GPU load. GPU busy, clocks and per-frame CPU submission/pacing times were not collected, so the slowdown cannot yet be assigned to emulation, driver cost or frontend pacing.

## Inputs And Method

- Apple M4, macOS 26.5.1, arm64. Sequential runs on the same host.
- Official RPCS3 `v0.0.43-20400`, engine commit `f848fd1e12aaba4f7c04bbd8642bfb9b5fae556c`; core based on local merge `b5e49b227`, with the uncommitted macOS integration changes described in [validation](MACOS_VALIDATION.md).
- Daytona USA Europe `NPEB00630`, firmware 4.92, LLVM PPU/SPU, Safe SPU block size, 1280×720 at 100%, Disable Vertex Cache enabled, PS3 Native frame limit. Existing private installations and persistent caches were reused; no new content copies were made.
- Both programs loaded the exact MoltenVK framework from the active RetroArch app, including its Xenia patch marker. The standalone used a process-local ICD selection and synchronous-submit setting. This is a driver-matched performance control, separate from the earlier successful standalone run using standard settings and bundled drivers.
- Standalone presentation used VSync Full. The core's internal VSync was disabled, with RetroArch video VSync enabled. The presentation paths and output window sizes differ; internal emulated resolution is equal.
- RPCS3's High performance overlay was sampled through timestamped CUA screenshots. Offline Vision OCR and reviewed numeric transcriptions are retained alongside the original PNGs. Host CPU/RSS was sampled every second; OCR ran between emulator cases.
- The automatic sequence runs after the first title/save prompts are dismissed. No driving input was sent during measurement. This is a comparison of the same repeated attract sequence, without deterministic frame alignment.

Each valid run has 14 samples. The standalone window spans 111.884 seconds and the core window 82.576 seconds because of observation cadence and the gap between screenshot batches. Scene proportions and durations therefore differ. The measurements establish an initial performance gap in this setup, not a precise general benchmark. Neither per-frame p95 nor 1% lows can be derived from these sparse overlay samples.

A preliminary core launch was excluded because an old per-core options file selected the Low overlay. The measured core runs use a fresh per-case configuration directory and verified High overlay. No samples from the preliminary launch enter the results.

## Async Control And Fault

The diagnostic core was built with only `VkBool32 synchronous` changed from `VK_TRUE` to `VK_FALSE` in the instance settings. LLVM, content, firmware, existing cache, options, frontend and driver were retained. The source was then restored and the synchronous build rebuilt. The diagnostic core was never installed in the user's RetroArch cores directory.

The regular Async run logged a host null read at approximately 0.393 seconds, during shader-cache startup. Two debugger repetitions stopped in the same function; the second captured all thread stacks. The full-stack run stopped on an invalid address rather than zero, so a stable null argument is not established.

```text
MoltenVKQueue-0-0-0.0-Dispatch
MVKImageView::populateMTLRenderPassAttachmentDescriptor + 28
MVKRenderSubpass::populateMTLRenderPassDescriptor
MVKCommandEncoder::beginMetalRenderPass
MVKCmdBeginRenderPass::encode
MVKCommandEncoder::encode
MVKCommandBuffer::submit
MVKQueueCommandBufferSubmission::execute
MVKQueue::submit [dispatch block]
```

This isolates asynchronous submission as the trigger in the tested core configuration; the earlier interpreter-only comparison could not do so. It is a host render-pass setup fault, distinct from the guest `_cellsurMixerMain` cold-cache fault observed previously in both core and standalone. There are no Async performance figures because it did not reach the measured scene.

[MoltenVK's documentation](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md#mvk_config_synchronous_queue_submits) explains that disabling synchronous submission moves queue processing from the calling thread to a GCD worker. The observed invalid image-view access on that worker is consistent with a resource-lifetime or ordering defect exposed by deferred encoding. The stack alone does not prove whether the invalidation originates in the core, shared RPCS3 Vulkan code or MoltenVK. The next diagnostic target is ownership and destruction of render-pass attachments during startup/loading, with Vulkan lifetime validation where available. An Async standalone control would also be needed before assigning the defect exclusively to the port.

The Xenia patches do not prevent this path from failing. “Async queue submission” here is separate from RPCS3's asynchronous shader recompilation, which remains unchanged in both controls.

## Evidence And Final State

Raw evidence is under [`build-macos-arm64/benchmark-2026-10-25/`](build-macos-arm64/benchmark-2026-10-25/):

- `summary.json`, `samples.csv`, `analyze.py`, `configuration-hashes.json`, `system-input-hashes.json`.
- `standalone-sync/` and `core-sync/`: manifests, frontend/emulator logs, loaded-library lists, timestamped PNG samples, OCR and host samples.
- `core-async/`: the non-debugger failure; `core-async-debug-first/` and `core-async-debug/`: debugger controls. `async-debug-stack.txt` contains the full stack capture.
- `core-sync-preflight/`: excluded preliminary run. `async-only.patch` describes the one-constant diagnostic change.

| Artifact | SHA-256 |
| --- | --- |
| Initial Measured Synchronous Core | `fea5912ecf7ff5ed8e7d1e16839de5ebf8f207cd1c2eb636ffd86a83d022eb56` |
| Diagnostic Async Core | `2a6edb17f3d4013e98b66b652e0023e6556b2c58c01a9bd1c8873d7b954be843` |
| Shared RetroArch MoltenVK | `6c511b765b68df9043b05fe6cdaacb78ce256115aab887a2b9d9c582143aa22b` |
| Game EBOOT, Both Installations | `3fd7ca7e78e4abd9893d9bb0a1e8d91762314edc7dce97bca2ca6b5f27a77e53` |

Firmware version-file, `liblv2.sprx` and game metadata hashes also match between the two installations; this is a targeted input check, not a full firmware-directory hash audit.

The core's normal Close Content / Quit path completed. The Async process can exit through RPCS3's fault handler without a new macOS crash report, and `open -W` returns zero even then; logs and debugger stops, rather than launcher status, establish the failure. A UI observation after the immediate Async exit reopened an empty RetroArch window; that window was closed and excluded from measurements.

Original RPCS3 support/cache directories and runner settings were restored. No emulator remains running. The source again requests synchronous submissions. The canonical installed `rpcs3_libretro.dylib`, matching `.info` and zstd library match the final distribution, and installed-core signing verification passed. No refs were published.

## Synchronous Timing-Log Investigation

The logging activity retained synchronous MoltenVK submission and added observation only: it did not change queue selection, image ownership, pacing or audio-output policy. Work is on `codex/sync-telemetry`; earlier macOS changes remain in its working tree. The local xemu reference is `ui/libretro.c` (`video_debug_log` and `retro_run`) and `docs/libretro-vulkan-presentation.md` in `/Users/andrea/dev/xemu-libretro`. It already documents a related shared-queue presentation stall and a dedicated-queue correction. Its emulator-specific patch is not copied into RPCS3.

### Controlled Result

Both controls use the same instrumented binary (`ba69cba2dabec649fd50fce47871f6947696ca65c35021336ed18a082c16f008`), installed driver, LLVM, firmware, content, cache, High overlay and synchronous host submission. Only RetroArch's private `video_vsync` value differs. Audio synchronization remains enabled. The game runs its automatic attract sequence without input or periodic UI capture during the selected windows.

Each column covers 49 complete one-second windows inside a 50-second selection, starting five seconds after the verified 3D-start screenshot. Automatic title/ranking transitions remain included, so these are sequence-throughput figures, not the earlier 3D-only overlay medians. Boundaries are aligned approximately from launcher time to RPCS3 log time; emulator startup introduces a sub-second uncertainty.

| Timing Log Measurement | VSync On | VSync Off, Diagnostic |
| --- | ---: | ---: |
| Selected Wall Time | 49.482 s | 49.346 s |
| Frontend Calls/s | 59.94 | 121.39 |
| Completed And Handed-Over Frames/s | 52.73 | 59.86 |
| Median One-Second Completed-Frame Rate | 53.15 | 60.00 |
| Duplicate Frontend Calls | 357 | 3036 |
| Dropped Frames / Step Detections | 0 / 0 | 0 / 0 |
| Renderer Queue-Lock Wait, CPU ms/s | 371.80 | 2.87 |
| Image Acquisition, CPU ms/s | 152.13 | 0.009 |
| Video Callback, CPU ms/s | 996.80 | 16.97 |
| Audio Callback, CPU ms/s | 0.153 | 0.176 |
| Audio Frames Offered And Accepted/s | 48006 | 48003 |

More frontend calls without VSync cause more duplicates, not faster guest simulation. The completed-frame rate remains around the guest's 60 Hz ceiling. GPU execution time is not measured. Cross-thread and nested CPU durations overlap and must not be added into a single utilization budget.

A separate five-second macOS process sample after the baseline measurement window places RSX in `vk::queue_submit → vk::libretro::lock_queue → __psynch_mutexwait`. The frontend's `vulkan_frame → vkQueueSubmit → MoltenVK render-pass encoding → getCAMetalDrawable → CAMetalLayer::nextDrawable` path is also sampled. This is consistent with synchronous frontend submission waiting for a drawable while it holds the lock needed by the renderer. The sample contains other waits too, including audio-driver waiting; it is not a claim that every main-thread sample is a drawable wait.

Together, the same-binary control, direct lock timing and stacks establish shared-queue presentation contention as a bottleneck in the tested setup. The engine can sustain approximately 60 completed frames/s when that presentation constraint is removed. This does not establish that every isolated low-FPS sample has the same cause or that CPU/GPU headroom is identical to standalone.

VSync Off was a diagnostic control. The subsequent [separate-queue correction](#separate-queue-correction) retains normal frontend VSync while isolating renderer submission from drawable acquisition, with explicit completion and image-reuse guarantees. The xemu dedicated-queue work supplied a design reference; RPCS3 uses its own completed-image handoff. Async host submission remains deferred.

### Using And Reading The Log

Enable **Show Advanced Options**, then **Diagnostics → Performance Timing Log** (`rpcs3_timing_log=enabled`) in Core Options. It defaults to `disabled`, is read at startup and on option updates, and is reset at content unload. No new runtime downloads, output locations or global settings are introduced.

Each `[RPCS3 Timing]` line represents about one second and includes:

- `host_us`: monotonic CPU timestamp for correlation; `window_s` and `runs`: denominator and call count.
- `run_hz`, `run_ms`, `run_p95_ms`, `period_p95_ms`, `outside_ms`: frontend cadence, core-call execution and time outside the call. Callback waits are included in core-call execution. Logging time is excluded from the next outside-time mean, but remains visible in call cadence.
- `shown_hz` and `dup`: images handed to the frontend versus requests to repeat its last image, not physical display changes. `vk_ready_hz`: GPU-completed images entering the hardware Vulkan handoff, not an arbitrary game FPS counter. The prototype control logs name this field `ready_hz`; the parser accepts both names.
- `dropped` and `steps`: frames discarded by the bounded Vulkan handoff and frontend pause/step detections. Startup, pauses and shutdown must be excluded from steady-state analysis.
- `audio_pull_ms`, `audio_cb_ms`, `audio_offered`, `audio_accepted`, `video_cb_ms`: sample extraction/conversion, frontend callback CPU time and offered/accepted audio sample frames.
- `take_ms`, `acquire_ms`, `acquire_max_ms`, `queue_wait_ms`, `queue_wait_max_ms`, `queue_hold_ms`, `fence_wait_ms`, `pause_wait_ms`: Vulkan handoff, free-image acquisition, renderer-side queue-lock acquisition/hold, presentation-completion fence waits and deliberate pause waits. The core cannot measure the frontend's lock hold directly; contention is observed from its waiting callers. With separate queues, `queue_wait_ms` is zero because that frontend mutex is not acquired; `queue_hold_ms` measures the renderer submit scope under RPCS3's own global lock.
- `frontend_wait_ms`, `handoff_submit_ms`: added by the separate-queue correction; CPU time in the frontend sync-index completion callback and the semaphore handoff submission. Older logs omit these fields.

Durations are totals in CPU milliseconds unless a mean/percentile/max is named. Per-window percentiles use up to the latest 512 calls, with `interval_samples` and `run_samples` reporting the retained counts. They are not a whole-run percentile or a GPU frame-time distribution. Cross-thread counter updates can straddle a window boundary. Frame-ready and queue/image/fence counters describe the hardware Vulkan path; frontend/audio callback timing also covers other presentation paths, whose runtime behavior was not tested in this activity.

Use the uncompressed engine log for timestamp selection:

```sh
python3 .ci/summarize-libretro-timing.py /path/to/rpcs3_detailed.log \
  --from-seconds 80 --to-seconds 130 --output timing.json
```

The helper checks that shown plus duplicate calls reconcile with frontend runs and that accepted audio does not exceed offered audio. It retains selected raw windows, combines rates by their actual durations, and labels CPU totals per wall second. It reports a median of window p95 values, not a reconstructed whole-run p95. Truncated records are ignored.

### Validation And Deployment

Evidence is in `build-macos-arm64/telemetry-2026-10-25/`: per-case logs/manifests/configurations, phase PNGs, raw host samples, baseline `process-sample.txt`, selected-window summaries and `comparison.json`. The two controls pass frame/audio counter reconciliation and close normally without forced termination. Runner settings are restored; no content is copied.

The final logging build (`e93a90e70316a3d919540458a3b9db15963a9a0dd2aefed9154923be29c05c05`) adds monotonic timestamps, explicit sample counts and the `vk_ready_hz` label. Its separate VSync-on check verifies 3D rendering, logging in both sinks, the visible Debug option, runtime disabling and normal cleanup. That build is not relabeled as the binary used for the controlled performance numbers above.

That logging-build deployment included the matching `.info` and zstd library, with backup/hash/signing evidence in the telemetry directory. The option remains disabled by default; the user's RetroArch configuration, core options and driver framework are preserved. That activity was a diagnostic update; the rendering correction and newer deployment are recorded below. Async was not tested in either follow-up.

## Separate-Queue Correction

The synchronous queue bottleneck is removed by reserving a frontend queue distinct from the renderer's queue whenever the GPU supports it. On the tested Apple M4, the renderer uses family 1 queue 0, RetroArch family 0 queue 0, and RPCS3 transfer work family 2. The selection first prefers a second queue within a graphics/compute family; otherwise it uses a second graphics/compute family. The transfer queue never shares the dedicated frontend queue. Devices without a spare queue retain the shared-queue fallback and its frontend mutex.

MoltenVK synchronous submissions, RetroArch VSync, audio synchronization, existing content/cache, engine revision, High overlay and internal resolution all remain unchanged. No new core option or global driver setting is introduced.

### Frame Ownership And Shutdown

Each completed renderer image is handed over with a binary semaphore dependency. With different queue families, only the frontend-facing images use concurrent sharing. Images remain in GENERAL layout, allowing repeated frontend reads without ownership/layout round trips. The frontend's current sync index is waited before reusing its semaphore; every sync index that samples an image is tracked, including duplicate frames. An image returns to the renderer only after all such readers complete and it is no longer the last displayed image. This follows the local Libretro Vulkan interface and [Vulkan's semaphore/fence ordering rules](https://docs.vulkan.org/spec/latest/chapters/synchronization.html).

Generation tokens accompany completed image indices. A reset or resize cannot turn an index from the previous image set into a reference to a newly allocated image. Retired images remain held until the frontend finishes their reads. The shared-queue fallback also checks generations when fetching/releasing an image.

Resize and shutdown submit empty fence markers to the involved queues under their respective submit locks, release those locks, and wait with a five-second timeout. They no longer call device-idle while holding RetroArch's mutex on the shared-device path. Upstream `Emu.GracefulShutdown` remains the emulation/thread shutdown path.

### Matched VSync-On Measurement

The candidate uses the same 50-second selection method as the earlier timing investigation: five seconds after a screenshot confirms the automatic 3D sequence, selecting complete one-second windows by engine log time. There are 49 selected windows, totaling 49.515 seconds. The repeated sequence includes automatic title/ranking transitions and is not deterministically frame-aligned. No UI input or repeated capture occurred during the selected candidate interval.

| Measurement | Previous Shared Queue, VSync On | Separate Queues, VSync On |
| --- | ---: | ---: |
| Completed Vulkan Images/s | 52.727 | 59.861 |
| New Images Handed To Frontend/s | 52.727 | 59.841 |
| Frontend Calls/s | 59.942 | 59.982 |
| Duplicate Calls In Selected Windows | 357 | 7 |
| Dropped Images | 0 | 0 |
| Renderer Wait For Frontend Queue Lock, CPU ms/s | 371.801 | 0.000 |
| Image Acquisition, CPU ms/s | 152.131 | 0.010 |
| Frontend Video Callback, CPU ms/s | 996.797 | 996.735 |
| Audio Accepted, Sample Frames/s | 48006.3 | 48001.9 |

The renderer recovers approximately 13.5% throughput, reaching the game's approximately 60 Hz target while VSync remains enabled. The foreground video callback still spends about one second per wall-clock second in display pacing; that no longer stops renderer submissions. The new frontend-completion wait costs 0.0014 CPU ms/s and the semaphore handoff submit 0.331 CPU ms/s in this interval. CPU times across threads and nested scopes overlap and are not additive.

The earlier same-build standalone overlay median was 59.96 FPS. This result closes the observed steady-state gap for the tested attract sequence; it does not establish equal headroom in every scene/game or identical latency. The new minimum one-second completed-image window is 56.09 Hz. Neither GPU busy nor physical scanout was measured.

### Validation And Limits

A separate diagnostic copy replaces only the core's Vulkan linkage with the local Vulkan loader, enabling Khronos API and synchronization validation while using the exact same installed MoltenVK framework as its ICD. This is a correctness run, not the performance sample. The previous installed core was relinked in the same way as a control.

The candidate reached 3D, paused/resumed through RetroArch, reset through Quick Menu, returned to 3D and closed content/quit normally. No synchronization hazards were reported. Both the previous and candidate builds reported these pre-existing API-validation categories:

- `VUID-vkDestroyFramebuffer-framebuffer-00892`: framebuffer destroyed while referenced by submitted work.
- `VUID-VkPipelineMultisampleStateCreateInfo-sampleShadingEnable-00784`: sample-rate shading feature not enabled.
- `VUID-VkImageViewCreateInfo-imageViewFormatSwizzle-04465`: portability-subset image-view swizzle feature not enabled.
- `VUID-VkSamplerCreateInfo-samplerMipLodBias-04467`: portability-subset sampler LOD-bias feature not enabled.

There were no additional error categories in the candidate run. The layer caps duplicate messages, and the candidate included two boots versus one in the control, so message counts are not defect-frequency comparisons. This is **not** a zero-error Vulkan validation claim. These existing reports remain separate follow-up work; the framebuffer lifetime report may help the later Async investigation, but its connection to that crash is not proven.

Raw evidence is in `build-macos-arm64/queues-2026-10-25/`: `candidate/summary.json` contains the selected raw windows and summary; each case has configuration, executable/content/driver hashes, logs and runtime manifest. `validation/` includes pause and before/after-reset screenshots. `baseline-validation/` contains the previous-core control. `validation-comparison.json` records error categories. The measured candidate core is SHA-256 `a75726dc663f9a5a37c0649c62628b9dd667d7811c1d112d8fde39c7b76fcf07`; the diagnostic loader copies have different hashes and are not installed.

### Final Build And Local Installation

A private diagnostic build forced the shared-queue fallback on the same Apple M4. It reached the same 3D sequence and closed normally. Automatic queue selection was then restored and the final build repeated the 3D/normal-close check with **two confirmed frontend swapchain images**, supplementing the three-image performance and validation runs. The same-family spare-queue topology is selected by capability but was not exercised on this one-queue-per-family GPU. Cross-platform and other-game behavior remain untested.

The final source adds a generation check in the shared fallback for a resize occurring between the initial check and `take_frame`; the dedicated-queue path is unchanged from the measured candidate. `verification.json` records successful frame/audio counter reconciliation for all five cases and normal exit without forced termination. No content was copied, and runner configuration was restored after each run.

Final installed core SHA-256: `0ce0ccdcb1e20e12b376e7cecfa5af9fd40abb60488779ba75916ed0ec3e31d1`. It is installed as `~/Library/Application Support/RetroArch/cores/rpcs3_libretro.dylib`, together with the matching canonical `.info` and zstd dependency; no `dev` suffix. The previous `e93a90...` core is preserved in `build-macos-arm64/queues-2026-10-25/deployment/backup/`. Atomic replacement, hashes, strict signing verification and unchanged driver/global configuration are recorded in `deployment/receipt.json`. The distribution in `build-macos-arm64/dist/` matches the installed files. Changes remain local and uncommitted on `codex/sync-telemetry`; no refs were published.


## Async Framebuffer Lifetime Investigation

The October 25 follow-up first tested the official standalone at the same upstream build 20400, reusing the private Daytona installation and persistent caches. Normal native UI and High overlay were retained unless a control explicitly changed them. MoltenVK Async means `MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0`; it is independent of RPCS3 asynchronous shader compilation.

### Standalone Controls

| Control | Observed Result |
| --- | --- |
| RetroArch's Xenia-patched MoltenVK, Async, normal UI and warm cache | Host fault during shader loading, before intro |
| Official app's bundled MoltenVK 1.4.2, Async | Host fault during startup; no 3D positive |
| RetroArch MoltenVK Async, native UI and overlay disabled, no GUI | Title reached; save-data flow blocked by unavailable Qt UI |
| Same Qt-UI control with GUI enabled | Title and load-data dialog reached; MainThread null fault after confirming load |
| RetroArch MoltenVK Async, on-disk shader cache disabled for this run | Title and native load-data dialog reached; host null fault before 3D |
| Official bundled KosmicKrisp, normal native UI | Daytona 3D attract mode reached, normal close, no recorded crash |

Disabling cache use did not delete caches. No unmodified MoltenVK Async standalone configuration produced a full 3D positive in these controls. The Qt MainThread crash is a separate observation and is not attributed to the framebuffer fix.

KosmicKrisp is a [Vulkan implementation on Metal](https://docs.mesa3d.org/drivers/kosmickrisp.html), not OpenGL. The explicit bundled ICD selected `Apple M4 (KosmicKrisp)`, driver `26.99.99`, library SHA-256 `74a77b01a86d1730222df11601c21e04d0e1d4ce8ad6fa93d7f136b4c789cb51`. Two retained 3D overlay samples read 16.31 and 13.12 FPS. Fresh driver caches, a different output size and concurrent compilation early in the run prevent a controlled performance comparison. The MoltenVK Async parameter does not apply to this driver. OpenGL was not tested.

### Cause And Correction

In the standard standalone Async control, LLDB captured `vkDestroyFramebuffer` called by `vk::remove_unused_framebuffers`, through `VKGSRender::flip` and the native shader-loading dialog. Immediately afterward, the MoltenVK dispatch worker stopped with a null access in `MVKImageView::populateMTLRenderPassAttachmentDescriptor`. This is the shared engine's framebuffer cache path, before normal game rendering; it is not specific to RetroArch presentation.

The old cache policy released a framebuffer after two idle CPU-reference checks. Overlay flips can advance those checks while GPU submissions still reference its attachments. The framebuffer also owns image views, explaining the asynchronous driver worker's invalid view access. The previous synchronous Vulkan-validation control already reported `VUID-vkDestroyFramebuffer-framebuffer-00892`.

Unused-cache eviction now transfers the framebuffer to RPCS3's existing resource manager, which retires it only after the corresponding submission event completes. Cache compaction remains linear and preserves order. Explicit image invalidation and final cache teardown retain their existing behavior. No queue-idle wait, pacing sleep, or core option was added.

### Core Results And Remaining Limitation

The unfixed Async core still faulted during startup. With deferred retirement, the private Async core reached the normal native UI and 3D attract sequence, ran for 293.63 seconds, and closed through Close Content/quit without forced termination. The existing warm caches and exact RetroArch MoltenVK provider were retained.

The 180–240 engine-second interval contains 59 complete timing windows totaling 59.897 seconds: 59.936 GPU-completed images/s, 60.020 frontend calls/s, but only 40.102 new images/s handed to the frontend, with 1,193 duplicate calls and 1,189 dropped images. The median window core-call p95 was 49.892 ms. Accepted audio averaged 48,005.6 sample frames/s. These counters reconcile; they do not measure physical scanout. This window samples the automatic sequence, including title transitions, rather than frame-aligned scenes. It establishes irregular Async delivery, not a throughput advantage over the earlier synchronous result (59.841 new images/s).

A second private Async build uses the final linear-compaction implementation and the local Vulkan loader with API/synchronization validation and the same MoltenVK ICD. It passed boot to 3D, pause/resume, reset back to 3D, and normal close. No `SYNC-HAZARD` or framebuffer `00892` report occurred. The three pre-existing feature-validation categories `00784`, `04465`, and `04467` remained; this is not a zero-error Vulkan validation result. The initial successful binary used per-item cache erasure; its hash is distinct from the final-compaction validation binary.

Evidence is under `build-macos-arm64/async-2026-10-25/`: `standalone-debug-stack.txt`, per-case runtime manifests/configs/logs, standalone KosmicKrisp screenshots, `core-async-fixed/timing-180-240.json`, and `core-async-validation/` lifecycle screenshots. Relevant SHA-256 values:

- Unfixed private Async core: `ecdc1665e0a19fb3a18ca314ddd9240cc8aac28f37f03acc4d1610191df1d1e9`.
- First successful Async core: `60822acb2bb548f06bbeab209ae87d5b07d4023d67d94d1842fe6e2a827de8b4`.
- Final-compaction Async validation copy: `6c2caac2c9f8fff31536a0bf34eacaa8247568cf4246a7652835dec4dea91543`.

The correction addresses the reproduced startup lifetime defect. It does not establish long-term Async reliability, fix the separate Qt crash, or fix frontend delivery cadence. The official standalone source/binary was not modified. Final local builds retain synchronous submissions and include the lifetime correction.


### Final Synchronous Build And Installation

The final build passed a separate non-validation Daytona 3D run and normal Close Content/quit (165.47 seconds, no forced termination). Its 110–160 engine-second selection contains 49 windows totaling 49.480 seconds: 59.903 completed/new images per second, 59.963 frontend calls/s, three duplicates, zero dropped images and 48,002.3 accepted audio sample frames/s. The median window core-call p95 is 16.961 ms. This regression check confirms regular presentation with the lifetime fix; it is not a frame-aligned Async/synchronous benchmark.

Core SHA-256 `4bce48f3bd90304a831b9b9326678b4494f4a1177d5543c0906825c08cc11531` is installed as `~/Library/Application Support/RetroArch/cores/rpcs3_libretro.dylib`, with the matching canonical `.info` and zstd dependency. `build-macos-arm64/dist/` matches the installation. The previous `0ce0ccd...` core is backed up under `async-2026-10-25/deployment/backup/`; `deployment/receipt.json` records atomic replacement, hashes and strict signature checks. RetroArch's global configuration and its MoltenVK framework remain unchanged. All owned test processes have stopped and redirected standalone profiles were restored.

Changes remain uncommitted on `codex/async-framebuffer-lifetime`, preserving the prior integration/queue work; no commit or push was performed. The next Async issue is frontend delivery cadence, separately from this corrected startup resource lifetime defect.


## Xemu Acquisition Fence Comparison

The October 25 same-binary control confirms that the private MoltenVK acquisition-fence correction tested in xemu-libretro also restores regular Async delivery in RPCS3. The framebuffer lifetime correction remains necessary in the core; this experiment addresses the separate presentation-cadence problem.

### Isolation And Inputs

Both runs use the exact same previously validated Async core with final framebuffer compaction (`6c2caac2c9f8fff31536a0bf34eacaa8247568cf4246a7652835dec4dea91543`), local Vulkan loader, RetroArch executable, private driver and Daytona fixture. The driver is the clean, non-instrumented xemu experiment, SHA-256 `2d1c48e06a9be7366e7f159476958d02027476fb88e5c5275ccc14beade4de9d`. It was copied into this project's ignored evidence directory; neither the xemu checkout nor RetroArch's installed MoltenVK was edited.

The candidate is based on MoltenVK 1.4.2 `e9b7bd2e4d6f365b36d29f5f57b8d39ca92f157f`, with the Xenia independent-queue and drawable ownership corrections required by the xemu experiment. It is not identical to the installed RetroArch driver, so the off case establishes a new control; its rate need not equal the earlier installed-driver 40.10 images/s result.

Only the private process environment switch `XEMU_MVK_ACQUIRE_FENCE_COMPLETION=1` changes between the measured runs. In the off case the variable is absent: the candidate tests variable presence, so setting it to `0` would still enable it. Engine YAML, core options and normalized frontend configuration were compared and matched. VSync, audio synchronization, three frontend swapchain images, High overlay, warm caches and separate renderer/frontend/transfer families 1/0/2 are retained. Logs identify asynchronous instance submission and the exact private ICD in both cases. No observer/interposer or diagnostic post-present CPU wait is loaded.

### Measured Result

After each screenshot confirms 3D, a 50-second interval starts five seconds later. Analysis includes only complete one-second records whose start and end both lie inside that interval. There is no further UI input or capture during measurement. Each selection has 49 records; the automatic attract sequence includes title transitions and is not deterministically frame-aligned.

| Measurement | Correction Off | Correction On |
| --- | ---: | ---: |
| Selected Duration, s | 49.763 | 49.530 |
| Frontend Calls/s | 59.984 | 59.984 |
| GPU-Completed Images/s | 59.883 | 59.883 |
| New Images Handed To Frontend/s | 33.559 | 59.883 |
| Duplicate Calls | 1,315 | 5 |
| Dropped Images | 1,310 | 0 |
| Median Of Window Core-Call p95, ms | 65.895 | 17.065 |
| Accepted Audio Sample Frames/s | 44,056.1 | 48,005.7 |

Both sessions reached visible 3D and completed normal Close Content/quit without forced termination (123.05 and 126.05 seconds). Renderer throughput barely changes while delivered-image rate recovers and long callback intervals disappear. Audio counters also recover; actual audio quality was not independently assessed. Physical scanout and GPU busy time were not measured, so new-image delivery is not relabeled as physical display FPS.

### Interpretation And Limits

The xemu candidate moves only a deferred acquisition's CPU fence from the later physical-presented callback to the predecessor's GPU command-buffer completion, retaining the existing drawable ownership and semaphore dependencies. The same-binary off/on RPCS3 control directly establishes that changing this dependency resolves the measured delivery deficit. It supports the same acquisition-fence/cadence mechanism seen in xemu; it does not imply that both cores respond identically to burst frontend calls. RPCS3 continued producing about 60 completed images/s while its bounded frontend handoff discarded excess frames.

No further emulation or handoff source change was needed for this comparison. The current production core still uses synchronous submission with the installed driver. The private candidate is retained for driver review, not installed globally or advertised as a general WSI conformance fix. Device loss, other present modes, acquisition fence reuse patterns and other applications remain outside this bounded result. The separate framebuffer use-after-free fixed earlier is not caused or repaired by the acquisition policy.

Evidence is in `build-macos-arm64/acquire-fence-2026-10-25/`: exact binaries and `inputs.json`, per-case environment/configuration/runtime/library records, screenshots, `MEASURE.json`, raw timing windows in each `summary.json`, and `comparison.json`. `run.py` and `measure.py` retain the launch/selection method; `analyze.py` checks normal close, provider identity and frame/audio counter consistency.


### Lifecycle, Visual Limitation And Local Deployment

With the candidate enabled, a separate API/synchronization-validation run confirms two actual frontend swapchain images, boot to 3D, pause/resume, reset back to 3D and normal close (273.50 seconds). It reports no `SYNC-HAZARD` or framebuffer `00892`. The known feature categories `00784`, `04465` and `04467` remain; this is not zero-error validation. A matched two-image validation control with the correction disabled also reaches 3D, pauses/resumes and closes normally (117.00 seconds), with the same three categories. Counts differ because the enabled run includes two boots.

`on-validation/resumed.png` captures a suspect scrambled car hood/windshield texture after resume. After reset, `on-validation/after-reset-later.png` shows the Hornet texture correctly. The disabled control does not reproduce the same anomaly in its retained captures, but the scenes and pause points are not frame-aligned; absence there does not establish that the fence change caused it. This remains a visual correctness question, separate from the measured cadence improvement, and prevents claiming an artifact-free candidate or readiness for global deployment.

The normal synchronous core is unchanged by this investigation. The verified canonical package (`4bce48f3bd90304a831b9b9326678b4494f4a1177d5543c0906825c08cc11531`) is copied again to local RetroArch with matching `.info`, backup and receipt in `acquire-fence-2026-10-25/deployment/`. The private Async core and private driver remain in the evidence directory. No driver, global RetroArch configuration, core option default or emulator source was changed; no commit or push was performed.


## Acquire Completion Revision 2: Bounded Application Check

The follow-up private driver `14c9f58d...` was tested once with the preserved Async/lifetime-fixed core, two confirmed frontend images and API/synchronization validation. It reached the title screen and closed normally after 173.32 seconds, but no 3D scene was confirmed in the retained observations. The prospective 50-second 3D measurement and pause/resume car-texture check were therefore not performed. A separately labeled post-hoc title-only interval shows 59.944 new images/s; this does not establish 3D performance or resolve the visual anomaly. Only the known sampleShadingEnable `00784` category was recorded (10 reports, duplicate-limited); no synchronization hazard or framebuffer `00892` was logged. No additional session was launched. See `build-macos-arm64/acquire-review-2026-10-25/REPORT.md` for exact identities, limits and evidence. Installed core/driver/global configuration remain unchanged.
