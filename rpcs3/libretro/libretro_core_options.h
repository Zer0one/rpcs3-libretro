#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"
#include "libretro_option_labels.h"

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition
 * - Translations live in libretro_core_options_intl.h, which is
 *   generated from Crowdin - edit only the English texts here
 */

struct retro_core_option_v2_category option_cats_us[] = {
    { "system", "System", "Language, region, content installation, compatibility database and disk cache." },
    { "cpu", "CPU", "Decoders, compilation and advanced CPU accuracy/workarounds." },
    { "gpu", "GPU", "Renderer, resolution, shaders and advanced graphics compatibility controls." },
    { "audio", "Audio", "Output level and the speaker layout presented to games." },
    { "network", "Network", "Network connectivity, PSN status, UPnP and DNS." },
    { "interface", "On-Screen Display", "Performance overlay, compilation hints and game notifications." },
    { "scheduling", "Scheduling (Advanced)", "Thread placement, compiler parallelism and guest thread scheduling." },
    { "timing", "Timing (Advanced)", "Guest clocks, VBlank, frame pacing, timers and original drive speeds." },
    { "diagnostics", "Diagnostics (Advanced)", "Performance measurements and logging controls for investigating problems." },
    { NULL, NULL, NULL }
};

// One group per function. Advanced status is handled by the display helper,
// without removing definitions or changing their saved values/defaults.
// Section-local visibility controls precede their targets, including when the
// frontend renders a flat API 0/1 menu. These preferences never change emulation values.
#define RPCS3_ADVANCED_SELECTOR(category, label) \
    { "rpcs3_" category "_show_advanced_options", label ": Show Advanced Options", "Show Advanced Options", \
      "Show advanced controls in this section only. Hiding them preserves their saved values and effects. Reopen Core Options if the frontend does not refresh immediately.", \
      NULL, category, { {"disabled", "Disabled"}, {"enabled", "Enabled"}, {NULL, NULL} }, "disabled" },

struct retro_core_option_v2_definition option_defs_us[] = {
    // ==================== SYSTEM ====================
    {
        "rpcs3_pkg_launcher", "System: PKG Launcher", "PKG Launcher",
        "Opening a base PS3 PSN game PKG starts its matching installation when present. The first load installs the game; reopen the PKG afterwards to play. Disable to use every PKG for installation only. Updates and DLC always remain installation-only. Applies the next time content is loaded; save this option before loading the PKG again.",
        NULL, "system",
        { {"enabled", "Enabled"}, {"disabled", "Disabled"}, {NULL, NULL} },
        // Port-only convenience: enabled keeps the PKG as the stable playlist
        // identity. Disabled restores standalone-style installation-only use.
        "enabled"
    },
    {
        "rpcs3_pkg_installed_playlist", "System: Installed PSN Playlist", "Installed PSN Playlist",
        "When a base PS3 PSN game PKG installs successfully or resolves to an existing installation, add or update its installed PARAM.SFO entry in Sony - PlayStation 3 (PSN Installed). The matching entry is labeled with the source PKG filename without its extension on every opening. Reopening the PKG can recreate a deleted playlist without reinstalling. Only playlists carrying the core's ownership marker are updated; a same-name user playlist is left unchanged with a notification. Existing entries are not duplicated. Independent of PKG Launcher; direct installed-game launch uses central or existing HDD RAP licenses. Disable to leave playlists untouched. Requires frontend playlist-directory support and uncompressed JSON playlists.",
        NULL, "system",
        { {"enabled", "Enabled"}, {"disabled", "Disabled"}, {NULL, NULL} },
        // Port-only post-install convenience, enabled as requested. No
        // standalone equivalent and no automatic modification of other lists.
        "enabled"
    },
    {
        "rpcs3_database_override", "System: Database Settings Override", "Database Settings Override",
        "Improves compatibility by automatically applying game-specific fixes: the settings RPCS3's database has for the game (as standalone RPCS3 applies them) take priority over the core options, global or per game, for the settings they name; every other setting stays as you set it. Off applies none of them. Takes effect the next time a game is started.",
        NULL, "system",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_language", "System: System Language", "System Language",
        "PS3 system language.",
        NULL, "system",
        { {"english", "English"}, {"japanese", "Japanese"}, {"french", "French"}, {"spanish", "Spanish"}, {"german", "German"}, {"italian", "Italian"}, {"dutch", "Dutch"}, {"portuguese", "Portuguese"}, {"russian", "Russian"}, {"korean", "Korean"}, {"chinese_trad", "Chinese (Traditional)"}, {"chinese_simp", "Chinese (Simplified)"}, {NULL, NULL} },
        "english"
    },
    {
        "rpcs3_license_area", "System: Console Region", "Console Region",
        "PS3 license region.",
        NULL, "system",
        { {"usa", "SCEA (Americas)"}, {"eu", "SCEE (Europe, Oceania)"}, {"jp", "SCEJ (Japan)"}, {"hk", "SCEH (Hong Kong, Southeast Asia)"}, {"kr", "SCEK (Korea)"}, {"cn", "SCH (China)"}, {NULL, NULL} },
        "usa"
    },
    {
        "rpcs3_enter_button", "System: Enter Button Assignment", "Enter Button Assignment",
        "Button used for confirm actions.",
        NULL, "system",
        { {"cross", "Cross (Western)"}, {"circle", "Circle (Japanese)"}, {NULL, NULL} },
        "cross"
    },
    {
        "rpcs3_disk_cache_size", "System: Disk Cache Size", "Disk Cache Size",
        "Limits the cache games keep on dev_hdd1, which otherwise only grows. With a size, the games' caches used longest ago are deleted when a game is loaded or closed, until the rest fits. Wipe on Exit deletes all of it when a game is closed, as a PS3 does when it is switched off. A deleted cache is built again by its game, which can make that game's next start slower.",
        NULL, "system",
        { {"unlimited", "Unlimited"}, {"1gb", "1 GB"}, {"5gb", "5 GB"}, {"wipe", "Wipe on Exit"}, {NULL, NULL} },
        "1gb"
    },
    // ==================== CPU ====================
    {
        "rpcs3_ppu_decoder", "CPU: PPU Decoder", "PPU Decoder",
        "PPU (main CPU) decoder. LLVM Recompiler is fastest.",
        NULL, "cpu",
        { {"llvm", "Recompiler (LLVM)"}, {"interpreter", "Interpreter (static)"}, {NULL, NULL} },
        "llvm"
    },
    {
        "rpcs3_spu_decoder", "CPU: SPU Decoder", "SPU Decoder",
        "SPU (co-processor) decoder. LLVM Recompiler is fastest.",
        NULL, "cpu",
        { {"llvm", "Recompiler (LLVM)"}, {"asmjit", "Recompiler (ASMJIT)"}, {"dynamic", "Interpreter (dynamic)"}, {"interpreter", "Interpreter (static)"}, {NULL, NULL} },
        "llvm"
    },
    {
        "rpcs3_llvm_precompilation", "CPU: PPU/SPU LLVM Precompilation", "PPU/SPU LLVM Precompilation",
        "Precompile PPU modules at boot for faster subsequent loads.",
        NULL, "cpu",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    RPCS3_ADVANCED_SELECTOR("cpu", "CPU")
    {
        "rpcs3_spu_xfloat_accuracy", "CPU: SPU XFloat Accuracy", "SPU XFloat Accuracy",
        "SPU floating-point accuracy level.",
        NULL, "cpu",
        { {"accurate", "Accurate"}, {"approximate", "Approximate"}, {"relaxed", "Relaxed"}, {"inaccurate", "Inaccurate"}, {NULL, NULL} },
        "approximate"
    },
    {
        "rpcs3_spu_block_size", "CPU: SPU Block Size", "SPU Block Size",
        "SPU recompiler block size. Mega/Giga may improve performance.",
        NULL, "cpu",
        { {"safe", "Safe"}, {"mega", "Mega"}, {"giga", "Giga"}, {NULL, NULL} },
        "safe"
    },
    {
        "rpcs3_accurate_rsx_reservation", "CPU: Accurate RSX Reservation Access", "Accurate RSX Reservation Access",
        "Synchronize RSX memory accesses with the reservations of the CPU threads. Needed by a few games, slower.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_spu_accurate_dma", "CPU: Accurate SPU DMA", "Accurate SPU DMA",
        "Accurately processes SPU DMA operations, as in RPCS3. Fixes a few games, at a large cost in speed.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_spu_accurate_reservations", "CPU: Accurate SPU Reservations", "Accurate SPU Reservations",
        "Accurately processes SPU reservations, as in RPCS3 (on by default there too). Turning it off can make some games faster and break others.",
        NULL, "cpu",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_ppu_reservation_priority", "CPU: PPU Reservation Priority", "PPU Reservation Priority",
        "Gives PPU reservations priority over the SPUs', as in RPCS3. Can help games that stall on them.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_mfc_delay_command", "CPU: Delay Each Odd MFC Command", "Delay Each Odd MFC Command",
        "Delays every second MFC (SPU DMA) command, as in RPCS3. Only for the few games that need it.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_getllar_spin_opt", "CPU: Disable SPU GETLLAR Spin Optimization", "Disable SPU GETLLAR Spin Optimization",
        "Turn off the detection of SPU busy-wait loops on GETLLAR. A few games need it off.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_spu_loop_detection", "CPU: Enable SPU Loop Detection", "Enable SPU Loop Detection",
        "Detects SPU loops that only wait and skips through them. Faster, but some games break with it; off by default, as in RPCS3. Worth turning on for speed on phones.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_spu_cache", "CPU: SPU Cache", "SPU Cache",
        "Enable SPU cache for faster subsequent loads.",
        NULL, "cpu",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_accurate_dfma", "CPU: Accurate DFMA", "Accurate DFMA",
        "Double-precision fused multiply-add done exactly, as in RPCS3 (on by default there too). Costs little on CPUs with FMA; turning it off can help on a slow phone, and breaks some games.",
        NULL, "cpu",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_spu_verification", "CPU: SPU Verification", "SPU Verification",
        "SPU code verification level.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_mfc_shuffling", "CPU: MFC Commands Shuffling", "MFC Commands Shuffling",
        "Shuffle MFC commands for accuracy.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_hle_lwmutex", "CPU: HLE lwmutex", "HLE lwmutex",
        "Use HLE implementation for lwmutex.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_hook_static_funcs", "CPU: Hook Static Functions", "Hook Static Functions",
        "Hook static functions for HLE.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_set_daz_ftz", "CPU: PPU Set DAZ and FTZ", "PPU Set DAZ and FTZ",
        "Sets the denormals-are-zero and flush-to-zero CPU flags for the PPU. For debugging.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_spu_cache_line_stores", "CPU: Accurate PPU/SPU Cache Line Stores", "Accurate PPU/SPU Cache Line Stores",
        "Enable accurate cache line stores.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_ppu_set_fpcc", "CPU: Accurate PPU Float Condition Control", "Accurate PPU Float Condition Control",
        "Accurately set PPU FPCC bits.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_ppu_set_sat_bit", "CPU: Accurate PPU Saturation Bit", "Accurate PPU Saturation Bit",
        "Accurately set PPU saturation bit.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_ppu_nj_mode", "CPU: Accurate PPU Non-Java Mode", "Accurate PPU Non-Java Mode",
        "PPU non-Java mode handling.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_ppu_accurate_vector_nan", "CPU: Accurate PPU Vector NaN Handling", "Accurate PPU Vector NaN Handling",
        "More accurate vector NaN handling.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_accurate_ppu_128", "CPU: Accurate PPU 128 Reservations", "Accurate PPU 128 Reservations",
        "Processes 128-byte PPU reservation operations accurately: never, always, or for loops up to the given length. For debugging.",
        NULL, "cpu",
        { {"0", "Never (Default)"}, {"-1", "Always"}, {"1", NULL}, {"2", NULL}, {"3", NULL}, {"4", NULL}, {"5", NULL}, {"6", NULL}, {"7", NULL}, {"8", NULL}, {"9", NULL}, {"10", NULL}, {"11", NULL}, {"12", NULL}, {"13", NULL}, {"14", NULL}, {NULL, NULL} },
        "0"
    },
    {
        "rpcs3_debug_console_mode", "CPU: Debug Console Mode", "Debug Console Mode",
        "Emulates a PS3 debug console (more memory and debug features) instead of a retail one. Only for software that needs it; not recommended otherwise.",
        NULL, "cpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    // ==================== GPU ====================
    {
        "rpcs3_renderer", "GPU: Renderer", "Renderer",
        "Vulkan renders on the frontend's Vulkan device and hands it each finished frame as an image; when the frontend is not running its Vulkan video driver, the frame is copied back through memory instead. OpenGL draws straight into the frontend's OpenGL context. (Restart required)",
        NULL, "gpu",
        { {"opengl", "OpenGL"}, {"vulkan", "Vulkan"}, {"null", "Null (No Video)"}, {NULL, NULL} },
        "vulkan"
    },
    {
        "rpcs3_aspect_ratio", "GPU: Aspect Ratio", "Aspect Ratio",
        "Changes output aspect ratio. Only some PS3 games support native 4:3. (Restart required)",
        NULL, "gpu",
        { {"16:9", "16:9 (default)"}, {"4:3", NULL}, {NULL, NULL} },
        "16:9"
    },
    {
        "rpcs3_default_resolution", "GPU: Default Resolution", "Default Resolution",
        "DO NOT CHANGE THIS OPTION TO UPSCALE THE GAME. Use Resolution Scale instead. Only use 1080p if the game supports it. Rarely due to emulation bugs some games will only render at resolutions below 720p.",
        NULL, "gpu",
        { {"720p", "720p (Default)"}, {"1080p", "1080p"}, {"480p", "480p"}, {"576p", "576p"}, {NULL, NULL} },
        "720p"
    },
    {
        "rpcs3_resolution_scale", "GPU: Resolution Scale", "Resolution Scale",
        "Internal rendering resolution as a percentage of the game's own. 200% of a 720p game is 2560x1440, of a 1080p one 3840x2160.",
        NULL, "gpu",
        { {"25", "25%"}, {"50", "50%"}, {"66", "66%"}, {"75", "75%"}, {"100", "100% (Native)"}, {"150", "150%"}, {"200", "200%"}, {"250", "250%"}, {"300", "300%"}, {"400", "400%"}, {"500", "500%"}, {"600", "600%"}, {"700", "700%"}, {"800", "800%"}, {NULL, NULL} },
        "100"
    },
    {
        "rpcs3_stretch_to_display", "GPU: Stretch to Display Area", "Stretch to Display Area",
        "Stretch game output to fill the display, reported to RetroArch at this aspect ratio: pick the display's for games patched to a wider one (21:9, 32:9). Needs RetroArch's aspect ratio set to Core Provided.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", "16:9"}, {"16:10", NULL}, {"21:9", NULL}, {"32:9", NULL}, {"4:3", NULL}, {"5:4", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_anisotropic_filter", "GPU: Anisotropic Filtering", "Anisotropic Filtering",
        "Texture filtering quality.",
        NULL, "gpu",
        { {"auto", "Auto"}, {"2", "2x"}, {"4", "4x"}, {"8", "8x"}, {"16", "16x"}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_msaa", "GPU: Anti-Aliasing", "Anti-Aliasing",
        "Multi-sample anti-aliasing where the game asks for it, as in RPCS3.",
        NULL, "gpu",
        { {"auto", "Auto"}, {"disabled", NULL}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_shader_mode", "GPU: Shader Mode", "Shader Mode",
        "How shaders are compiled. Async compiles in the background but the frame that first needs a pipeline waits for it, which is what a long freeze on a new scene usually is. Async with Shader Interpreter draws that frame through the interpreter instead and swaps in the compiled shader when it is ready: no freeze, lower speed while it catches up.",
        NULL, "gpu",
        { {"async", "Async (Recommended)"}, {"async_interpreter", "Async with Shader Interpreter (no stalls)"}, {"async_recompiler", "Async with Recompiler"}, {"interpreter", "Shader Interpreter only"}, {"sync", "Synchronous"}, {NULL, NULL} },
        "async"
    },
    RPCS3_ADVANCED_SELECTOR("gpu", "GPU")
#ifdef __APPLE__
    {
        "rpcs3_moltenvk_queue_submission", "GPU: MoltenVK Queue Submission", "MoltenVK Queue Submission",
        "Choose Sync or Async submission for the macOS Vulkan frontend. Sync is the validated default; Async requires a compatible MoltenVK driver and remains experimental. Restart RetroArch after changing this option. This is independent of asynchronous shader compilation.",
        NULL, "gpu",
        { {"sync", "Sync"}, {"async", "Async"}, {NULL, NULL} },
        // Keep the validated Sync baseline until Async 3D and pause/resume
        // correctness have been confirmed with the selected driver.
        "sync"
    },
#endif
    {
        "rpcs3_zcull_accuracy", "GPU: ZCULL Accuracy", "ZCULL Accuracy",
        "How exactly occlusion queries are answered, as in RPCS3. Precise is correct and the default there; Approximate and Relaxed are faster, and can break effects such as lens flares. Relaxed is worth trying on phones.",
        NULL, "gpu",
        { {"precise", "Precise (Default)"}, {"approximate", "Approximate"}, {"relaxed", "Relaxed (Fastest)"}, {NULL, NULL} },
        "precise"
    },
    {
        "rpcs3_shader_quality", "GPU: Shader Quality", "Shader Quality",
        "Precision of the shaders RPCS3 generates. Low is fastest, Ultra most accurate.",
        NULL, "gpu",
        { {"low", "Low"}, {"high", "High"}, {"ultra", "Ultra"}, {NULL, NULL} },
        "high"
    },
    {
        "rpcs3_scale_threshold", "GPU: Resolution Scale Threshold", "Resolution Scale Threshold",
        "Render targets smaller than this are not scaled. Some games need a different value for the scaling to look right.",
        NULL, "gpu",
        { {"1", "1x1"}, {"16", "16x16 (Default)"}, {"64", "64x64"}, {"128", "128x128"}, {"160", "160x160"}, {"256", "256x256"}, {"320", "320x320"}, {"512", "512x512"}, {"592", "592x592"}, {"640", "640x640"}, {"1024", "1024x1024"}, {NULL, NULL} },
        "16"
    },
    {
        "rpcs3_write_color_buffers", "GPU: Write Color Buffers", "Write Color Buffers",
        "Write color buffers to main memory. Fixes some effects.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_write_depth_buffers", "GPU: Write Depth Buffers", "Write Depth Buffers",
        "Write depth buffers to main memory.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_read_color_buffers", "GPU: Read Color Buffers", "Read Color Buffers",
        "Read color buffers from main memory.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_read_depth_buffers", "GPU: Read Depth Buffers", "Read Depth Buffers",
        "Read depth buffers from main memory.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_strict_rendering", "GPU: Strict Rendering Mode", "Strict Rendering Mode",
        "Enable strict rendering for accuracy.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_multithreaded_rsx", "GPU: Multithreaded RSX", "Multithreaded RSX",
        "Moves part of the RSX work to a second thread. Off by default, as in RPCS3; it can help on CPUs with few fast cores, phones included.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_async_texture_streaming", "GPU: Asynchronous Texture Streaming", "Asynchronous Texture Streaming",
        "Enable asynchronous texture streaming.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_host_gpu_labels", "GPU: Allow Host GPU Labels", "Allow Host GPU Labels",
        "Lets the host GPU signal RSX labels itself instead of the CPU waiting on them. Faster in some games, unstable in others.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_blit_upscaling", "GPU: Disable Blit Engine Upscaling", "Disable Blit Engine Upscaling",
        "As in RPCS3: copies made by the RSX blit engine are left at the game's own resolution instead of the Resolution Scale. Fixes some games' effects at higher scales, at the cost of a blurrier picture where they are used.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_vertex_cache", "GPU: Disable Vertex Cache", "Disable Vertex Cache",
        "Turn off the cache of vertex data, as in RPCS3. Slower; only for games that show broken or stale geometry with it.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_emulate_depth_compare", "GPU: Emulate Special Depth Comparison", "Emulate Special Depth Comparison",
        "Emulates the depth comparison modes the PS3 has and the host GPU lacks. Fixes some shadows, at a cost in speed.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_force_hw_msaa_resolve", "GPU: Force Hardware MSAA Resolve", "Force Hardware MSAA Resolve",
        "Resolves anti-aliased surfaces on the GPU's own hardware instead of a shader. Faster, but some games show artifacts with it.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_handle_tiled_memory", "GPU: Handle RSX Memory Tiling", "Handle RSX Memory Tiling",
        "Emulates the RSX's tiled memory, for games that read their tiled surfaces back. Costs speed.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_use_rebar", "GPU: Use Re-BAR Memory for GPU Uploads", "Use Re-BAR Memory for GPU Uploads",
        "Uploads to the GPU through resizable BAR memory where the Vulkan driver offers it, as in RPCS3 (on by default there too). Turn off if it causes problems.",
        NULL, "gpu",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_rsx_fifo_accuracy", "GPU: RSX FIFO Accuracy", "RSX FIFO Accuracy",
        "RSX FIFO command accuracy level.",
        NULL, "gpu",
        { {"fast", "Fast"}, {"atomic", "Atomic"}, {"atomic_ordered", "Ordered & Atomic"}, {"as_ps3", "PS3"}, {NULL, NULL} },
        "atomic"
    },
    {
        "rpcs3_driver_wakeup_delay", "GPU: Driver Wake-Up Delay", "Driver Wake-Up Delay",
        "Driver wake-up delay in microseconds. 0 by default, as in RPCS3; raise it only when a game needs it.",
        NULL, "gpu",
        { {"0", "0 (Default)"}, {"20", "20"}, {"50", "50"}, {"100", "100"}, {"200", "200"}, {"400", "400"}, {"800", "800"}, {NULL, NULL} },
        "0"
    },
    {
        "rpcs3_driver_recovery_timeout", "GPU: Driver Recovery Timeout", "Driver Recovery Timeout",
        "GPU driver recovery timeout in milliseconds.",
        NULL, "gpu",
        { {"0", "Disabled"}, {"1000", "1 second"}, {"2000", "2 seconds"}, {"5000", "5 seconds"}, {"10000", "10 seconds"}, {NULL, NULL} },
        "1000"
    },
    {
        "rpcs3_disable_fifo_reordering", "GPU: Disable FIFO Reordering", "Disable FIFO Reordering",
        "Turns off the reordering of RSX commands RPCS3 does for speed. For debugging.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_hw_blending", "GPU: Disable Hardware Blending", "Disable Hardware Blending",
        "Forces programmable blending on renderers that support it, as in RPCS3's Debug tab. Purely a debugging option.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_hw_colorspace", "GPU: Disable Hardware ColorSpace Remapping", "Disable Hardware ColorSpace Remapping",
        "Remaps texture colour spaces in shaders instead of in the GPU's hardware. For debugging.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_disable_zcull_queries", "GPU: Disable ZCull Occlusion Queries", "Disable ZCull Occlusion Queries",
        "Report occlusion queries as passed without running them. Faster; breaks games that rely on them (lens flares, visibility).",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_cpu_blit", "GPU: Force CPU Blit Emulation", "Force CPU Blit Emulation",
        "Force CPU blit emulation for certain effects.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_gpu_texture_scaling", "GPU: Force GPU Texture Scaling", "Force GPU Texture Scaling",
        "Scales textures on the GPU instead of the CPU. For debugging; can break games.",
        NULL, "gpu",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_fb_aliasing_bias", "GPU: Framebuffer Aliasing Heuristic Bias", "Framebuffer Aliasing Heuristic Bias",
        "Which way RPCS3 decides a surface used as both colour and depth. For debugging.",
        NULL, "gpu",
        { {"auto", "Auto"}, {"color", "Prefer Color"}, {"depth", "Prefer Depth"}, {NULL, NULL} },
        "auto"
    },
    // ==================== AUDIO ====================
    {
        "rpcs3_master_volume", "Audio: Master Volume", "Master Volume",
        "Master audio volume percentage.",
        NULL, "audio",
        { {"0", "0%"}, {"10", "10%"}, {"20", "20%"}, {"30", "30%"}, {"40", "40%"}, {"50", "50%"}, {"60", "60%"}, {"70", "70%"}, {"80", "80%"}, {"90", "90%"}, {"100", "100%"}, {NULL, NULL} },
        "100"
    },
    {
        "rpcs3_audio_format", "Audio: Audio Format", "Audio Format",
        "The speakers the PS3 tells games it has. Set it to RetroArch's Output Layout: a game picks its own stereo, 5.1 or 7.1 mix from what is offered, and a surround mix folded down by RetroArch does not always sound like the game's own stereo one. Needs a RetroArch with multi-channel audio; stereo otherwise. Takes effect when a game boots.",
        NULL, "audio",
        { {"stereo", "Stereo"}, {"surround_5_1", "Surround 5.1"}, {"surround_7_1", "Surround 7.1"}, {NULL, NULL} },
        "stereo"
    },
    // ==================== NETWORK ====================
    {
        "rpcs3_network_enabled", "Network: Network Enabled", "Network Enabled",
        "Lets games reach the internet. Takes effect when content is loaded.",
        NULL, "network",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_psn_status", "Network: PSN Status", "PSN Status",
        "What games see of the PlayStation Network. Simulated makes them believe they are signed in, which some need to get past their menus; RPCN connects to RPCS3's own server and needs Network Enabled and an RPCN account: the core cannot create one, so set it up in standalone RPCS3 and copy its rpcn.yml into system/rpcs3/ (system/rpcs3/config/ on Windows). Takes effect when content is loaded.",
        NULL, "network",
        { {"disabled", NULL}, {"simulated", "Simulated"}, {"rpcn", "RPCN"}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_upnp", "Network: UPnP", "UPnP",
        "Enable UPnP for automatic port forwarding.",
        NULL, "network",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_dns", "Network: DNS Server", "DNS Server",
        "DNS server address.",
        NULL, "network",
        { {"8.8.8.8", "Google DNS"}, {"1.1.1.1", "Cloudflare DNS"}, {"208.67.222.222", "OpenDNS"}, {NULL, NULL} },
        "8.8.8.8"
    },
    // ==================== ON-SCREEN DISPLAY ====================
    {
        "rpcs3_perf_overlay", "On-Screen Display: Performance Overlay", "Performance Overlay",
        "RPCS3's own performance overlay, drawn over the game: the game's frame rate as RPCS3 counts it, and at higher detail levels frame times and CPU and GPU load.",
        NULL, "interface",
        { {"disabled", NULL}, {"minimal", "Frame rate only"}, {"low", "Low"}, {"medium", "Medium"}, {"high", "High"}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_show_ppu_compilation_hint", "On-Screen Display: Show PPU Compilation Hint", "Show PPU Compilation Hint",
        "Show hint when PPU modules are being compiled.",
        NULL, "interface",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_show_shader_compilation_hint", "On-Screen Display: Show Shader Compilation Hint", "Show Shader Compilation Hint",
        "Show hint when shaders are being compiled.",
        NULL, "interface",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_show_trophy_popups", "On-Screen Display: Show Trophy Popups", "Show Trophy Popups",
        "Show trophy unlock notifications.",
        NULL, "interface",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_show_rpcn_popups", "On-Screen Display: Show RPCN Popups", "Show RPCN Popups",
        "Show RPCN notification popups.",
        NULL, "interface",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    // Root-level control between the normal and advanced-only sections.
    {
        "rpcs3_show_advanced_sections", "Show Advanced Sections", NULL,
        "Show Scheduling (Advanced), Timing (Advanced) and Diagnostics (Advanced). These sections contain no internal visibility selector. CPU and GPU keep their own independent Show Advanced Options controls. Hiding sections preserves every saved value and effect.",
        NULL, NULL,
        { {"disabled", "Disabled"}, {"enabled", "Enabled"}, {NULL, NULL} },
        "disabled"
    },
    // ==================== SCHEDULING ====================
    {
        "rpcs3_thread_scheduler", "Scheduling (Advanced): Thread Scheduler", "Thread Scheduler",
        "Who places the emulator's threads on the CPU's cores, as in RPCS3. Operating System leaves it to the system; the RPCS3 schedulers pin PPU, SPU and RSX threads to cores of their own, laid out for a few desktop CPUs. Has no effect on Android, where the core cannot pin threads.",
        NULL, "scheduling",
        { {"os", "Operating System"}, {"old", "RPCS3 Scheduler"}, {"alt", "RPCS3 Alternative Scheduler"}, {NULL, NULL} },
        "os"
    },
    {
        "rpcs3_preferred_spu_threads", "Scheduling (Advanced): Preferred SPU Threads", "Preferred SPU Threads",
        "Number of SPU threads. Auto recommended.",
        NULL, "scheduling",
        { {"0", "Auto"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {NULL, NULL} },
        "0"
    },
    {
        "rpcs3_llvm_threads", "Scheduling (Advanced): PPU/SPU LLVM Compiler Threads", "PPU/SPU LLVM Compiler Threads",
        "How many threads compile PPU and SPU code with LLVM, as RPCS3's Max LLVM Compile Threads. Auto uses every core; fewer keeps a phone cooler and uses less memory while a game is compiled, and makes it take longer. On ARM CPUs with efficiency cores (most phones, some ARM laptops) the default leaves those out.",
        NULL, "scheduling",
        { {"0", "Auto"}, {"1", NULL}, {"2", NULL}, {"3", NULL}, {"4", NULL}, {"6", NULL}, {"8", NULL}, {NULL, NULL} },
        "0"
    },
    {
        "rpcs3_shader_compiler_threads", "Scheduling (Advanced): Shader Compiler Threads", "Shader Compiler Threads",
        "How many threads compile shaders. Auto lets RPCS3 decide from the CPU it sees. On ARM CPUs with efficiency cores (most phones, some ARM laptops) the default leaves those out.",
        NULL, "scheduling",
        { {"auto", "Auto"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"6", "6"}, {"8", "8"}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_ppu_threads", "Scheduling (Advanced): PPU Thread Count", "PPU Thread Count",
        "How many PPU threads run at once, as in RPCS3. 2 is the default and what nearly every game wants.",
        NULL, "scheduling",
        { {"1", NULL}, {"2", "2 (Default)"}, {"3", NULL}, {"4", NULL}, {"5", NULL}, {"6", NULL}, {"7", NULL}, {"8", NULL}, {NULL, NULL} },
        "2"
    },
    {
        "rpcs3_max_spurs_threads", "Scheduling (Advanced): Maximum Number of SPURS Threads", "Maximum Number of SPURS Threads",
        "Maximum SPURS thread count. Lower may improve performance.",
        NULL, "scheduling",
        { {"auto", "Auto"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_max_preempt_count", "Scheduling (Advanced): Max Power Saving CPU-Preemptions", "Max Power Saving CPU-Preemptions",
        "How many times a frame RPCS3 may pause the CPU threads to save power, as in RPCS3. 0 turns it off.",
        NULL, "scheduling",
        { {"0", "0 (Disabled)"}, {"10", NULL}, {"20", NULL}, {"50", NULL}, {"100", NULL}, {"200", NULL}, {"400", NULL}, {NULL, NULL} },
        "0"
    },
    {
        "rpcs3_spu_delay_penalty", "Scheduling (Advanced): SPU Delay Penalty", "SPU Delay Penalty",
        "SPU delay penalty for scheduling.",
        NULL, "scheduling",
        { {"0", "0"}, {"1", "1"}, {"2", "2"}, {"3", "3 (Default)"}, {"4", "4"}, {"5", "5"}, {NULL, NULL} },
        "3"
    },
    {
        "rpcs3_spu_events_busy_loop", "Scheduling (Advanced): Enable SPU Events Busy Loop", "Enable SPU Events Busy Loop",
        "Lets SPU threads spin while they wait for events instead of sleeping. Can help a few games, costs CPU time.",
        NULL, "scheduling",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    // ==================== TIMING ====================
    {
        "rpcs3_frame_pacing", "Timing (Advanced): Frame Pacing", "Frame Pacing",
        "What the game's frames are timed by. Emulator clock is RPCS3's own timer, as in standalone RPCS3. RetroArch gives the game one PS3 refresh per frame RetroArch shows, so fast-forward speeds the game up and the game runs at the VBlank Frequency when RetroArch shows that many frames a second.",
        NULL, "timing",
        { {"frontend", "RetroArch"}, {"emulator", "Emulator clock"}, {NULL, NULL} },
        "emulator"
    },
    {
        "rpcs3_frame_limit", "Timing (Advanced): Frame Limit", "Frame Limit",
        "Highest frame rate the game may run at. Auto is the VBlank Frequency, as in RPCS3; PS3 Native paces flips the way the PS3 does; Off lets games that do not wait for the PS3's refresh themselves run too fast.",
        NULL, "timing",
        { {"auto", "Auto"}, {"ps3", "PS3 Native"}, {"off", "Off"}, {"30", "30 FPS"}, {"50", "50 FPS"}, {"60", "60 FPS"}, {"120", "120 FPS"}, {"144", "144 FPS"}, {"240", "240 FPS"}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_vblank_rate", "Timing (Advanced): VBlank Frequency", "VBlank Frequency",
        "The PS3's refresh rate in Hz. It is also the frame rate the core asks RetroArch for, which takes effect when content is loaded; games that can run above 60 FPS need a display that refreshes that fast. \"From List\" doubles Vblank rate for supported games listed in vblank_unlock.txt found in system/rpcs3. You may need to apply per-game patches.",
        NULL, "timing",
        { {"50", "50 Hz (PAL)"}, {"60", "60 Hz (NTSC)"}, {"120", "120 Hz"}, {"144", "144 Hz"}, {"240", "240 Hz"}, {"list", "From List"}, {NULL, NULL} },
        "60"
    },
    {
        "rpcs3_vblank_ntsc", "Timing (Advanced): VBlank NTSC Fixup", "VBlank NTSC Fixup",
        "Run the VBlank Frequency at the NTSC rate (59.94 Hz for 60) instead of the round number. On by default, as in RPCS3.",
        NULL, "timing",
        { {"enabled", NULL}, {"disabled", NULL}, {NULL, NULL} },
        "enabled"
    },
    {
        "rpcs3_clocks_scale", "Timing (Advanced): Clocks Scale", "Clocks Scale",
        "Scale PS3 clock speed percentage.",
        NULL, "timing",
        { {"50", "50%"}, {"75", "75%"}, {"100", "100%"}, {"150", "150%"}, {"200", "200%"}, {"300", "300%"}, {NULL, NULL} },
        "100"
    },
    {
        "rpcs3_sleep_timers_accuracy", "Timing (Advanced): Sleep Timers Accuracy", "Sleep Timers Accuracy",
        "Sleep timers accuracy level.",
        NULL, "timing",
        { {"auto", "Automatic"}, {"as_host", "As Host"}, {"usleep", "Usleep Only"}, {"all_timers", "All Timers"}, {NULL, NULL} },
        "auto"
    },
    {
        "rpcs3_emulate_hdd_speed", "Timing (Advanced): Emulate HDD Read Speed", "Emulate HDD Read Speed",
        "Reads from the emulated hard disk at the speed of the PS3's, for games that break when loading is instant.",
        NULL, "timing",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_emulate_bdvd_speed", "Timing (Advanced): Emulate BD-ROM Read Speed", "Emulate BD-ROM Read Speed",
        "Reads from the disc at the PS3 drive's 2x speed (9 MB/s), for games that stream movies from disc and show corrupted video when loading is instant. Loading takes longer.",
        NULL, "timing",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    // ==================== DIAGNOSTICS ====================
    {
        "rpcs3_timing_log", "Diagnostics (Advanced): Performance Timing Log", "Performance Timing Log",
        "Writes one diagnostic summary per second to RetroArch and rpcs3_detailed.log: frontend cadence, shown/duplicate frames and audio/video callback times; Vulkan hardware completed/dropped frames, queue contention, image acquisition and fence waits. CPU wall times, not GPU execution time or physical presentation. Disabled by default to avoid measurement overhead.",
        NULL, "diagnostics",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    {
        "rpcs3_silence_all_logs", "Diagnostics (Advanced): Silence All Logs", "Silence All Logs",
        "Silence all log output for performance.",
        NULL, "diagnostics",
        { {"disabled", NULL}, {"enabled", NULL}, {NULL, NULL} },
        "disabled"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL }
};

#undef RPCS3_ADVANCED_SELECTOR

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us,       /* RETRO_LANGUAGE_ENGLISH */
   &options_ja,       /* RETRO_LANGUAGE_JAPANESE */
   &options_fr,       /* RETRO_LANGUAGE_FRENCH */
   &options_es,       /* RETRO_LANGUAGE_SPANISH */
   &options_de,       /* RETRO_LANGUAGE_GERMAN */
   &options_it,       /* RETRO_LANGUAGE_ITALIAN */
   &options_nl,       /* RETRO_LANGUAGE_DUTCH */
   &options_pt_br,    /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   &options_pt_pt,    /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   &options_ru,       /* RETRO_LANGUAGE_RUSSIAN */
   &options_ko,       /* RETRO_LANGUAGE_KOREAN */
   &options_cht,      /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   &options_chs,      /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   &options_eo,       /* RETRO_LANGUAGE_ESPERANTO */
   &options_pl,       /* RETRO_LANGUAGE_POLISH */
   &options_vn,       /* RETRO_LANGUAGE_VIETNAMESE */
   &options_ar,       /* RETRO_LANGUAGE_ARABIC */
   &options_el,       /* RETRO_LANGUAGE_GREEK */
   &options_tr,       /* RETRO_LANGUAGE_TURKISH */
   &options_sk,       /* RETRO_LANGUAGE_SLOVAK */
   &options_fa,       /* RETRO_LANGUAGE_PERSIAN */
   &options_he,       /* RETRO_LANGUAGE_HEBREW */
   &options_ast,      /* RETRO_LANGUAGE_ASTURIAN */
   &options_fi,       /* RETRO_LANGUAGE_FINNISH */
   &options_id,       /* RETRO_LANGUAGE_INDONESIAN */
   &options_sv,       /* RETRO_LANGUAGE_SWEDISH */
   &options_uk,       /* RETRO_LANGUAGE_UKRAINIAN */
   &options_cs,       /* RETRO_LANGUAGE_CZECH */
   &options_val,      /* RETRO_LANGUAGE_CATALAN_VALENCIA */
   &options_ca,       /* RETRO_LANGUAGE_CATALAN */
   &options_en,       /* RETRO_LANGUAGE_BRITISH_ENGLISH */
   &options_hu,       /* RETRO_LANGUAGE_HUNGARIAN */
   &options_be,       /* RETRO_LANGUAGE_BELARUSIAN */
   &options_gl,       /* RETRO_LANGUAGE_GALICIAN */
   &options_no,       /* RETRO_LANGUAGE_NORWEGIAN */
   &options_ga,       /* RETRO_LANGUAGE_IRISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static inline void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   libretro_option_labels::mark_defaults(options_us.definitions);

#ifndef HAVE_NO_LANGEXTRA
   /* Generated translations predate categorized short labels. Preserve their
    * existing translated names while the English schema supplies the groups.
    * No generated translation file or Crowdin workflow needs changing. */
   for (unsigned lang = 0; lang < RETRO_LANGUAGE_LAST; ++lang)
      if (options_intl[lang] && options_intl[lang] != &options_us)
         for (struct retro_core_option_v2_definition *def = options_intl[lang]->definitions;
              def && def->key; ++def)
         {
            if (!def->desc_categorized)
               def->desc_categorized = def->desc;
         }
   for (unsigned lang = 0; lang < RETRO_LANGUAGE_LAST; ++lang)
      if (options_intl[lang] && options_intl[lang] != &options_us)
         libretro_option_labels::mark_defaults(options_intl[lang]->definitions, options_us.definitions);
#endif

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      /* Use the published list, which may include per-title patches/status. */
      struct retro_core_option_v2_definition
            *published_defs_us          = options_us.definitions;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (published_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from published_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &published_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from published_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = published_defs_us[i].key;
            const char *desc                       = published_defs_us[i].desc;
            const char *default_value              = published_defs_us[i].default_value;
            struct retro_core_option_value *values = published_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  desc = libretro_option_labels::legacy_description(published_defs_us[i], default_index);
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
