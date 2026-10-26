#pragma once

#include "libretro.h"

// Libretro has no advanced flag. Keep this menu-only policy separate from the
// emulation configuration: hidden option values must still be read/applied.
namespace libretro_option_visibility
{
inline constexpr const char* advanced_keys[] = {
    "rpcs3_spu_xfloat_accuracy",
    "rpcs3_spu_block_size",
    "rpcs3_accurate_rsx_reservation",
    "rpcs3_spu_accurate_dma",
    "rpcs3_spu_accurate_reservations",
    "rpcs3_ppu_reservation_priority",
    "rpcs3_mfc_delay_command",
    "rpcs3_disable_getllar_spin_opt",
    "rpcs3_spu_loop_detection",
    "rpcs3_spu_cache",
    "rpcs3_accurate_dfma",
    "rpcs3_spu_verification",
    "rpcs3_mfc_shuffling",
    "rpcs3_hle_lwmutex",
    "rpcs3_hook_static_funcs",
    "rpcs3_set_daz_ftz",
    "rpcs3_spu_cache_line_stores",
    "rpcs3_ppu_set_fpcc",
    "rpcs3_ppu_set_sat_bit",
    "rpcs3_ppu_nj_mode",
    "rpcs3_ppu_accurate_vector_nan",
    "rpcs3_accurate_ppu_128",
    "rpcs3_debug_console_mode",
#ifdef __APPLE__
    "rpcs3_moltenvk_queue_submission",
#endif
    "rpcs3_zcull_accuracy",
    "rpcs3_shader_quality",
    "rpcs3_scale_threshold",
    "rpcs3_write_color_buffers",
    "rpcs3_write_depth_buffers",
    "rpcs3_read_color_buffers",
    "rpcs3_read_depth_buffers",
    "rpcs3_strict_rendering",
    "rpcs3_multithreaded_rsx",
    "rpcs3_async_texture_streaming",
    "rpcs3_host_gpu_labels",
    "rpcs3_disable_blit_upscaling",
    "rpcs3_disable_vertex_cache",
    "rpcs3_emulate_depth_compare",
    "rpcs3_force_hw_msaa_resolve",
    "rpcs3_handle_tiled_memory",
    "rpcs3_use_rebar",
    "rpcs3_rsx_fifo_accuracy",
    "rpcs3_driver_wakeup_delay",
    "rpcs3_driver_recovery_timeout",
    "rpcs3_disable_fifo_reordering",
    "rpcs3_disable_hw_blending",
    "rpcs3_disable_hw_colorspace",
    "rpcs3_disable_zcull_queries",
    "rpcs3_cpu_blit",
    "rpcs3_gpu_texture_scaling",
    "rpcs3_fb_aliasing_bias",
    "rpcs3_thread_scheduler",
    "rpcs3_preferred_spu_threads",
    "rpcs3_llvm_threads",
    "rpcs3_shader_compiler_threads",
    "rpcs3_ppu_threads",
    "rpcs3_max_spurs_threads",
    "rpcs3_max_preempt_count",
    "rpcs3_spu_delay_penalty",
    "rpcs3_spu_events_busy_loop",
    "rpcs3_frame_pacing",
    "rpcs3_frame_limit",
    "rpcs3_vblank_rate",
    "rpcs3_vblank_ntsc",
    "rpcs3_clocks_scale",
    "rpcs3_sleep_timers_accuracy",
    "rpcs3_emulate_hdd_speed",
    "rpcs3_emulate_bdvd_speed",
    "rpcs3_timing_log",
    "rpcs3_silence_all_logs",
};

// Failed early environment calls must not latch state: a subsequent frontend
// publication/update can retry once the display interface becomes available.
inline bool update(retro_environment_t cb, bool show, bool force = false)
{
    static bool valid = false;
    static bool previous = false;
    if (!cb || (!force && valid && previous == show))
        return false;

    bool accepted = false;
    bool complete = true;
    for (const char* key : advanced_keys)
    {
        retro_core_option_display display{key, show};
        const bool result = cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
        accepted |= result;
        complete &= result;
    }
    previous = show;
    valid = complete;
    return accepted;
}
}
