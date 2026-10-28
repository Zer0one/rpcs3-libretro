#pragma once

#include "libretro.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <vector>

// Libretro has no advanced flag. Keep this menu-only policy separate from the
// emulation configuration: hidden option values must still be read/applied.
namespace libretro_option_visibility
{
inline constexpr const char* advanced_sections_key = "rpcs3_show_advanced_sections";
struct section
{
    const char* category;
    const char* selector;
};

inline constexpr section sections[] = {
    {"cpu", "rpcs3_cpu_show_advanced_options"},
    {"gpu", "rpcs3_gpu_show_advanced_options"},
};

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
inline const section* section_for(const char* category)
{
    if (category)
        for (const auto& section : sections)
            if (!std::strcmp(category, section.category))
                return &section;
    return nullptr;
}

inline const retro_core_option_v2_definition* definition_for(
    const retro_core_option_v2_definition* definitions, const char* key)
{
    for (auto def = definitions; def && def->key; ++def)
        if (!std::strcmp(key, def->key))
            return def;
    return nullptr;
}

inline bool advanced_only(const char* category, const retro_core_option_v2_definition* definitions)
{
    if (!category)
        return false;
    const auto* section = section_for(category);
    bool found = false;
    for (auto def = definitions; def && def->key; ++def)
    {
        if (!def->category_key || std::strcmp(category, def->category_key) ||
            (section && !std::strcmp(section->selector, def->key)))
            continue;
        if (std::none_of(std::begin(advanced_keys), std::end(advanced_keys),
            [def](const char* key) { return !std::strcmp(key, def->key); }))
            return false;
        found = true;
    }
    return found;
}

// Run after adding game-dependent categories too, so Patch Manager cannot
// accidentally appear after the sections containing only advanced controls.
inline void move_advanced_sections_last(std::vector<retro_core_option_v2_definition>& options,
    std::vector<retro_core_option_v2_category>& categories,
    const retro_core_option_v2_definition* definitions)
{
    const auto after_normal = std::stable_partition(options.begin(), options.end(), [definitions](const auto& option)
    {
        return std::strcmp(option.key, advanced_sections_key) && !advanced_only(option.category_key, definitions);
    });
    std::stable_partition(after_normal, options.end(), [](const auto& option)
    {
        return !std::strcmp(option.key, advanced_sections_key);
    });
    std::stable_partition(categories.begin(), categories.end(), [definitions](const auto& category)
    {
        return !advanced_only(category.key, definitions);
    });
}

using enabled_getter = bool (*)(const char* key);

inline bool update(retro_environment_t cb, const retro_core_option_v2_definition* definitions,
    enabled_getter enabled, bool force = false)
{
    constexpr auto count = sizeof(sections) / sizeof(sections[0]);
    static std::array<bool, count> valid{};
    static std::array<bool, count> previous{};
    static bool advanced_sections_valid = false;
    static bool advanced_sections_previous = false;
    if (!cb || !definitions || !enabled)
        return false;

    bool accepted = false;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto& section = sections[index];
        const bool show = enabled(section.selector);
        if (!force && valid[index] && previous[index] == show)
            continue;
        bool complete = true;
        bool found = false;
        for (const char* key : advanced_keys)
        {
            const auto* def = definition_for(definitions, key);
            if (!def || !def->category_key || std::strcmp(section.category, def->category_key))
                continue;
            found = true;
            retro_core_option_display display{key, show};
            const bool result = cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            accepted |= result;
            complete &= result;
        }
        previous[index] = show;
        valid[index] = complete && found;
    }
    // Dedicated (Advanced) sections have no internal toggle. Their shared root
    // switch hides/shows all children, which also hides/shows the RA categories.
    const bool show_sections = enabled(advanced_sections_key);
    if (force || !advanced_sections_valid || advanced_sections_previous != show_sections)
    {
        bool complete = true;
        for (const char* key : advanced_keys)
        {
            const auto* def = definition_for(definitions, key);
            if (!def || !advanced_only(def->category_key, definitions))
                continue;
            retro_core_option_display display{key, show_sections};
            const bool result = cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &display);
            accepted |= result;
            complete &= result;
        }
        advanced_sections_valid = complete;
        advanced_sections_previous = show_sections;
    }
    return accepted;
}
}
