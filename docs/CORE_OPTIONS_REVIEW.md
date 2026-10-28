# Core Options Review

Review Date: 2026-10-26. This review covers the core changes included in v0.1.0.2 and the official standalone sources at **v0.0.43-20400**, commit `f848fd1e12aaba4f7c04bbd8642bfb9b5fae556c`.

## Libretro Conventions And Functional Organization

The [deeper source comparison](ADVANCED_OPTIONS.md) documents real show/hide selectors in Snes9x, Genesis Plus GX and PCSX ReARMed. Those implementations use the selectors for flat menus and hide the selectors when V2 categories are available. RPCS3 keeps independent section-local selectors available with categories too, without splitting CPU/GPU into standard and advanced menus.

**Show Advanced Options** appears immediately before the advanced controls inside CPU and GPU. A root **Show Advanced Sections** control follows the normal sections, including the always-present Patch Manager, and shows/hides Scheduling (Advanced), Timing (Advanced) and Diagnostics (Advanced) at the end. Those dedicated sections contain no internal selector. All three visibility preferences default to Disabled. Option classification, saved values and effects are unchanged; RetroArch's global advanced-settings preference is independent.

The v0.1.0.2 review counted **99 fixed options on macOS, 98 elsewhere**, including the new selector. All 98 previous macOS keys, allowed values and defaults were retained. The subsequent Installed PSN Playlist option adds one normal System control, bringing those totals to **100 on macOS, 99 elsewhere**; it is a port-only convenience and does not change the historical standalone-field audit below. The final layout replaces the former global visibility control with two mixed-section controls and a root advanced-section gate, for current totals of **102 fixed options on macOS, 101 elsewhere**. Patch Manager adds game-dependent patch controls, or one informational status row when empty. English flat-menu labels carry functional prefixes; V2 categorized menus use short names. Existing translated short labels are retained and new category names fall back to English. Generated translation files and disabled Crowdin workflows are unchanged.

| Group | Purpose |
| --- | --- |
| System | PKG launcher, installed PSN playlist, compatibility database, language, region, confirm button and disk cache |
| CPU | Decoders, compilation, and advanced accuracy/CPU workarounds |
| GPU | Renderer, image settings, and advanced graphics/driver controls |
| Audio | Volume and the speaker layout advertised to games |
| Network | Internet/PSN status, UPnP and DNS |
| On-Screen Display | Performance overlay, compilation hints and notifications |
| Patch Manager | Per-title patch controls, or an informational availability row |
| Scheduling (Advanced) | Advanced thread placement, compiler parallelism and guest scheduling |
| Timing (Advanced) | Advanced frame pacing/limits, VBlank, clocks, timers and original drive speeds |
| Diagnostics (Advanced) | Advanced timing summaries and log suppression |

The eight formerly permanently hidden settings are now controlled by their section selectors: SPU Cache, Accurate DFMA, SPU Verification, MFC Commands Shuffling and HLE lwmutex (CPU); Driver Recovery Timeout (GPU); SPU Delay Penalty (Scheduling); VBlank NTSC Fixup (Timing). Patch Manager retains its existing independent game-specific/dependent visibility.

## Standalone Cross-Check

The official [settings dialog](https://github.com/RPCS3/rpcs3/blob/f848fd1e12aaba4f7c04bbd8642bfb9b5fae556c/rpcs3/rpcs3qt/settings_dialog.ui) has ten tabs: **CPU, GPU, Audio, I/O, System, Network, Advanced, Emulator, GUI and Debug**. The core follows their functional areas while distributing Advanced/Debug controls by what they affect. GUI/host-window tabs are not copied into the core.

The [standalone settings registry](https://github.com/RPCS3/rpcs3/blob/f848fd1e12aaba4f7c04bbd8642bfb9b5fae556c/rpcs3/rpcs3qt/emu_settings_type.cpp) contains **213 distinct mapped config fields** after deduplicating its repeated `AccurateSpuReservations` entry. The [full cross-reference CSV](CORE_OPTIONS_STANDALONE.csv) records each field, its core option where present, main-dialog reference and disposition.

| Disposition | Standalone Fields | Meaning |
| --- | ---: | --- |
| Exposed | 93 | Connected to a core option, sometimes consolidated |
| Not Exposed | 69 | Candidates for review, including specialist diagnostics/cosmetics |
| Frontend Or Port Policy | 22 | Frontend-controlled or deliberately fixed behavior |
| Backend Work Required | 20 | Input, peripherals or other backend work before a selector is useful |
| Standalone UI | 6 | Qt/host-window facilities with no direct core equivalent |
| Save-State Work Required | 3 | Depend on Libretro serialization and lifecycle support |

These are field counts, not a feature-completeness percentage: multiple standalone fields map to one core option, while eleven core options in the v0.1.0.2 audit were port-specific or exposed config fields outside this registry. The CSV is a settings-dialog/registry audit, not an exhaustive inventory of every RPCS3 YAML key. A main-dialog reference records a source reference, not proof that a widget is enabled for every renderer/platform. Library overrides are managed separately and are discussed below.

### Compatibility Candidates To Review First

| Area | Not Exposed | Suggested Group | Implementation Considerations |
| --- | --- | --- | --- |
| SPU | SPU MMIO Delay Enabled | CPU | Real SPU consumers exist. Consider the separate config-only delay value too; a boolean alone follows the upstream default delay |
| PPU | PPU Event Wake-Up Delay | Timing | Real event-queue consumer; use upstream units/range |
| PPU | Accurate PPU Vector Estimates (`AccuratePPUVEST`) | CPU | Separate from XFloat and the already exposed Vector NaN setting |
| GPU | Strict Texture Flushing | GPU | Real texture-cache behavior; may cost performance |
| GPU | Force High Precision Z Buffer | GPU | Both OpenGL and Vulkan have consumers; renderer capabilities apply |
| macOS | Disable MSL Fast Math | GPU | Must reach the actual frontend Vulkan-instance creation path; setting a config field alone is insufficient |
| Vulkan | Asynchronous Scheduler Driver | GPU | Distinct from shader compilation, asynchronous texture streaming and MoltenVK queue submission |
| GPU | Disable Async Host Memory Manager | GPU | Upstream internal memory/synchronization setting; validate separately before offering it |
| Libraries | HLE/LLE library overrides | CPU, Or A Dedicated Library Group | Managed through `libraries_control`, outside the 213-field enum map; needs an actionable per-library interface |

No new missing emulation selector is implemented by this review. These are source-backed candidates, not verified fixes for a particular game or Async failure.

### Other Gaps

- **Graphics:** texture LOD bias, stereo/anaglyph configuration, shader-cache disabling, Vulkan allocator debugging and additional rendering diagnostics. Internal FSR/output filtering is fixed or replaced by frontend scaling/shaders; that is not an exact equivalent of every upstream filter.
- **Network:** bind address, IP swap list, PSN country, MAC derivation from PSID and clans. RPCN account credentials/configuration are a separate concern from exposing these selectors.
- **System:** keyboard layout, date/time format and console-time offset, HDD identity, temporary-directory cleanup and host-root access. Host-root access changes filesystem exposure and needs an explicit product decision.
- **Audio:** guest audio-provider/RSXAudio-port selection needs a support review. Microphone and music selectors need working backends; the current music callback returns `null_music_handler`. Audio dumping is a separate diagnostic candidate.
- **Input/Peripherals:** keyboard and mouse callbacks instantiate null handlers, and the camera callback returns `null_camera_handler`. Move, Buzz, turntable, guitar, MIDI/USIO and motion options require actual Libretro input/peripheral integration. Do not expose standalone host-handler names as inert options.
- **Overlay/Diagnostics:** graph appearance, font/opacity/position, additional hints, CPU/SPU/MFC debug, profiling, shader logging, RenderDoc and per-channel logs. These are lower priority than compatibility controls unless investigating a concrete issue.
- **Save States:** the standalone mode preferences do not provide Libretro Save State support. They require serialization and real frontend round-trip validation first.

### Existing Differences And A Corrected Connection

The cross-check found that **Max Power Saving CPU-Preemptions** applied its selected value and then reset it to zero at the end of `libretro_apply_core_options`. The redundant reset has been removed; the default remains zero. This is the only emulation-setting behavior changed by the menu organization.

The upstream GUI's `DisableHWBlending` and `DisableHWTexelRemapping` enum names target the opposite config entries. The core already maps its labels to the actual behavior; this review preserves that mapping instead of copying the misleading enum names.

Existing default differences remain unchanged: the core defaults to asynchronous shader recompilation while standalone uses asynchronous recompilation with the interpreter; the core limits disk cache to 1 GB while standalone does not enable that limit; PPU compilation hints are off in the core and on upstream. These should be kept explicit in future parity work. The PPU/SPU decoder choices match the decoder enums at this build; there is no missing dynamic PPU interpreter choice.

Frontend VSync, fullscreen/window behavior, final scaling, output-device selection and extra audio buffering/time stretching are not missing emulation toggles to copy blindly. The core intentionally uses the frontend for those responsibilities.

## Historical v0.1.0.2 Validation And Limits

- Native schema audit: unique keys, valid defaults, category membership/order, complete API 2/1/0 registration, English/Italian fallback and `HAVE_NO_LANGEXTRA`.
- Before/after native contract comparison: all 98 existing macOS keys, allowed-value lists and defaults are identical; the new visibility selector is the only additional key.
- Native hide/show tests cover repeated updates, forced republication and retries after unavailable display calls.
- Existing PKG launcher and RAP/disc-key fixture checks pass.
- macOS ARM64 core build, dependency audit and code-signature verification pass.
- No emulator or game was launched. Menu rendering and non-default preemption behavior have not been runtime-tested; other-platform binaries were not built by this activity.

Local build, source-reference hashes, native checks, canonical RetroArch deployment receipt and backup are under `build-macos-arm64/options-organization/`. The implementation is included in v0.1.0.2; the runtime limits above remain applicable.

## Fixed Option Inventory

The MoltenVK option is macOS-only. CPU/GPU advanced controls are visible when their own section's Show Advanced Options is enabled; dedicated (Advanced) sections follow the root Show Advanced Sections gate. All settings keep their saved values while hidden.

| Group | Option | Advanced | Saved Key |
| --- | --- | --- | --- |
| System | PKG Launcher | No | `rpcs3_pkg_launcher` |
| System | Installed PSN Playlist | No | `rpcs3_pkg_installed_playlist` |
| System | Database Settings Override | No | `rpcs3_database_override` |
| System | System Language | No | `rpcs3_language` |
| System | Console Region | No | `rpcs3_license_area` |
| System | Enter Button Assignment | No | `rpcs3_enter_button` |
| System | Disk Cache Size | No | `rpcs3_disk_cache_size` |
| CPU | PPU Decoder | No | `rpcs3_ppu_decoder` |
| CPU | SPU Decoder | No | `rpcs3_spu_decoder` |
| CPU | PPU/SPU LLVM Precompilation | No | `rpcs3_llvm_precompilation` |
| CPU | Show Advanced Options | No | `rpcs3_cpu_show_advanced_options` |
| CPU | SPU XFloat Accuracy | Yes | `rpcs3_spu_xfloat_accuracy` |
| CPU | SPU Block Size | Yes | `rpcs3_spu_block_size` |
| CPU | Accurate RSX Reservation Access | Yes | `rpcs3_accurate_rsx_reservation` |
| CPU | Accurate SPU DMA | Yes | `rpcs3_spu_accurate_dma` |
| CPU | Accurate SPU Reservations | Yes | `rpcs3_spu_accurate_reservations` |
| CPU | PPU Reservation Priority | Yes | `rpcs3_ppu_reservation_priority` |
| CPU | Delay Each Odd MFC Command | Yes | `rpcs3_mfc_delay_command` |
| CPU | Disable SPU GETLLAR Spin Optimization | Yes | `rpcs3_disable_getllar_spin_opt` |
| CPU | Enable SPU Loop Detection | Yes | `rpcs3_spu_loop_detection` |
| CPU | SPU Cache | Yes | `rpcs3_spu_cache` |
| CPU | Accurate DFMA | Yes | `rpcs3_accurate_dfma` |
| CPU | SPU Verification | Yes | `rpcs3_spu_verification` |
| CPU | MFC Commands Shuffling | Yes | `rpcs3_mfc_shuffling` |
| CPU | HLE lwmutex | Yes | `rpcs3_hle_lwmutex` |
| CPU | Hook Static Functions | Yes | `rpcs3_hook_static_funcs` |
| CPU | PPU Set DAZ and FTZ | Yes | `rpcs3_set_daz_ftz` |
| CPU | Accurate PPU/SPU Cache Line Stores | Yes | `rpcs3_spu_cache_line_stores` |
| CPU | Accurate PPU Float Condition Control | Yes | `rpcs3_ppu_set_fpcc` |
| CPU | Accurate PPU Saturation Bit | Yes | `rpcs3_ppu_set_sat_bit` |
| CPU | Accurate PPU Non-Java Mode | Yes | `rpcs3_ppu_nj_mode` |
| CPU | Accurate PPU Vector NaN Handling | Yes | `rpcs3_ppu_accurate_vector_nan` |
| CPU | Accurate PPU 128 Reservations | Yes | `rpcs3_accurate_ppu_128` |
| CPU | Debug Console Mode | Yes | `rpcs3_debug_console_mode` |
| GPU | Renderer | No | `rpcs3_renderer` |
| GPU | Aspect Ratio | No | `rpcs3_aspect_ratio` |
| GPU | Default Resolution | No | `rpcs3_default_resolution` |
| GPU | Resolution Scale | No | `rpcs3_resolution_scale` |
| GPU | Stretch to Display Area | No | `rpcs3_stretch_to_display` |
| GPU | Anisotropic Filtering | No | `rpcs3_anisotropic_filter` |
| GPU | Anti-Aliasing | No | `rpcs3_msaa` |
| GPU | Shader Mode | No | `rpcs3_shader_mode` |
| GPU | Show Advanced Options | No | `rpcs3_gpu_show_advanced_options` |
| GPU | MoltenVK Queue Submission | Yes | `rpcs3_moltenvk_queue_submission` |
| GPU | ZCULL Accuracy | Yes | `rpcs3_zcull_accuracy` |
| GPU | Shader Quality | Yes | `rpcs3_shader_quality` |
| GPU | Resolution Scale Threshold | Yes | `rpcs3_scale_threshold` |
| GPU | Write Color Buffers | Yes | `rpcs3_write_color_buffers` |
| GPU | Write Depth Buffers | Yes | `rpcs3_write_depth_buffers` |
| GPU | Read Color Buffers | Yes | `rpcs3_read_color_buffers` |
| GPU | Read Depth Buffers | Yes | `rpcs3_read_depth_buffers` |
| GPU | Strict Rendering Mode | Yes | `rpcs3_strict_rendering` |
| GPU | Multithreaded RSX | Yes | `rpcs3_multithreaded_rsx` |
| GPU | Asynchronous Texture Streaming | Yes | `rpcs3_async_texture_streaming` |
| GPU | Allow Host GPU Labels | Yes | `rpcs3_host_gpu_labels` |
| GPU | Disable Blit Engine Upscaling | Yes | `rpcs3_disable_blit_upscaling` |
| GPU | Disable Vertex Cache | Yes | `rpcs3_disable_vertex_cache` |
| GPU | Emulate Special Depth Comparison | Yes | `rpcs3_emulate_depth_compare` |
| GPU | Force Hardware MSAA Resolve | Yes | `rpcs3_force_hw_msaa_resolve` |
| GPU | Handle RSX Memory Tiling | Yes | `rpcs3_handle_tiled_memory` |
| GPU | Use Re-BAR Memory for GPU Uploads | Yes | `rpcs3_use_rebar` |
| GPU | RSX FIFO Accuracy | Yes | `rpcs3_rsx_fifo_accuracy` |
| GPU | Driver Wake-Up Delay | Yes | `rpcs3_driver_wakeup_delay` |
| GPU | Driver Recovery Timeout | Yes | `rpcs3_driver_recovery_timeout` |
| GPU | Disable FIFO Reordering | Yes | `rpcs3_disable_fifo_reordering` |
| GPU | Disable Hardware Blending | Yes | `rpcs3_disable_hw_blending` |
| GPU | Disable Hardware ColorSpace Remapping | Yes | `rpcs3_disable_hw_colorspace` |
| GPU | Disable ZCull Occlusion Queries | Yes | `rpcs3_disable_zcull_queries` |
| GPU | Force CPU Blit Emulation | Yes | `rpcs3_cpu_blit` |
| GPU | Force GPU Texture Scaling | Yes | `rpcs3_gpu_texture_scaling` |
| GPU | Framebuffer Aliasing Heuristic Bias | Yes | `rpcs3_fb_aliasing_bias` |
| Audio | Master Volume | No | `rpcs3_master_volume` |
| Audio | Audio Format | No | `rpcs3_audio_format` |
| Network | Network Enabled | No | `rpcs3_network_enabled` |
| Network | PSN Status | No | `rpcs3_psn_status` |
| Network | UPnP | No | `rpcs3_upnp` |
| Network | DNS Server | No | `rpcs3_dns` |
| On-Screen Display | Performance Overlay | No | `rpcs3_perf_overlay` |
| On-Screen Display | Show PPU Compilation Hint | No | `rpcs3_show_ppu_compilation_hint` |
| On-Screen Display | Show Shader Compilation Hint | No | `rpcs3_show_shader_compilation_hint` |
| On-Screen Display | Show Trophy Popups | No | `rpcs3_show_trophy_popups` |
| On-Screen Display | Show RPCN Popups | No | `rpcs3_show_rpcn_popups` |
| Top Level | Show Advanced Sections | No | `rpcs3_show_advanced_sections` |
| Scheduling (Advanced) | Thread Scheduler | Yes | `rpcs3_thread_scheduler` |
| Scheduling (Advanced) | Preferred SPU Threads | Yes | `rpcs3_preferred_spu_threads` |
| Scheduling (Advanced) | PPU/SPU LLVM Compiler Threads | Yes | `rpcs3_llvm_threads` |
| Scheduling (Advanced) | Shader Compiler Threads | Yes | `rpcs3_shader_compiler_threads` |
| Scheduling (Advanced) | PPU Thread Count | Yes | `rpcs3_ppu_threads` |
| Scheduling (Advanced) | Maximum Number of SPURS Threads | Yes | `rpcs3_max_spurs_threads` |
| Scheduling (Advanced) | Max Power Saving CPU-Preemptions | Yes | `rpcs3_max_preempt_count` |
| Scheduling (Advanced) | SPU Delay Penalty | Yes | `rpcs3_spu_delay_penalty` |
| Scheduling (Advanced) | Enable SPU Events Busy Loop | Yes | `rpcs3_spu_events_busy_loop` |
| Timing (Advanced) | Frame Pacing | Yes | `rpcs3_frame_pacing` |
| Timing (Advanced) | Frame Limit | Yes | `rpcs3_frame_limit` |
| Timing (Advanced) | VBlank Frequency | Yes | `rpcs3_vblank_rate` |
| Timing (Advanced) | VBlank NTSC Fixup | Yes | `rpcs3_vblank_ntsc` |
| Timing (Advanced) | Clocks Scale | Yes | `rpcs3_clocks_scale` |
| Timing (Advanced) | Sleep Timers Accuracy | Yes | `rpcs3_sleep_timers_accuracy` |
| Timing (Advanced) | Emulate HDD Read Speed | Yes | `rpcs3_emulate_hdd_speed` |
| Timing (Advanced) | Emulate BD-ROM Read Speed | Yes | `rpcs3_emulate_bdvd_speed` |
| Diagnostics (Advanced) | Performance Timing Log | Yes | `rpcs3_timing_log` |
| Diagnostics (Advanced) | Silence All Logs | Yes | `rpcs3_silence_all_logs` |
