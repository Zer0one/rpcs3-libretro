// Link against the built core to exercise RPCS3's real configuration schema.
#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/RSX/VK/vkutils/device.h"
#include <cassert>
#include <iostream>

int main()
{
    const auto original = g_cfg.to_string();
    // Model effective settings after hardware fallbacks and initialization.
    g_cfg.video.antialiasing_level.set(msaa_level::none);
    g_cfg.video.shadermode.set(shader_mode::async_recompiler);
    g_cfg.video.precise_zpass_count.set(false);
    g_cfg.video.vk.asynchronous_scheduler.set(vk_gpu_scheduler_mode::safe);
    g_cfg.video.resolution_scale_percent.set(100);
    g_cfg.audio.volume.set(100);
    g_cfg.core.spu_decoder.set(spu_decoder_type::dynamic);

    const std::string requested = R"(Core:
  SPU Decoder: Recompiler (LLVM)
Video:
  MSAA: Auto
  Shader Mode: Shader Interpreter only
  Accurate ZCULL stats: true
  Resolution Scale: 150
  Vulkan:
    Asynchronous Queue Scheduler: Fast
Audio:
  Master Volume: 75
)";
    assert(g_cfg.from_string(requested, true));
    assert(g_cfg.video.antialiasing_level.get() == msaa_level::none);
    assert(g_cfg.video.shadermode.get() == shader_mode::async_recompiler);
    assert(!g_cfg.video.precise_zpass_count.get());
    assert(g_cfg.video.vk.asynchronous_scheduler.get() == vk_gpu_scheduler_mode::safe);
    assert(g_cfg.core.spu_decoder.get() == spu_decoder_type::dynamic);
    assert(g_cfg.video.resolution_scale_percent.get() == 150);
    assert(g_cfg.audio.volume.get() == 75);
    const auto effective = g_cfg.to_string();
    assert(g_cfg.from_string(requested, true));
    assert(g_cfg.to_string() == effective);

    // The same request becomes eligible when a new session is initialized.
    assert(g_cfg.from_string(requested, false));
    assert(g_cfg.video.antialiasing_level.get() == msaa_level::_auto);
    assert(g_cfg.video.precise_zpass_count.get());
    assert(g_cfg.video.shadermode.get() == shader_mode::interpreter_only);
    assert(g_cfg.video.vk.asynchronous_scheduler.get() == vk_gpu_scheduler_mode::fast);
    assert(g_cfg.core.spu_decoder.get() == spu_decoder_type::llvm);
    // An empty logical-device capability set must reject both requests.
    // This exercises the real guard without claiming a physical-GPU test.
    const vk::render_device no_enabled_features;
    no_enabled_features.apply_config_limits();
    assert(g_cfg.video.antialiasing_level.get() == msaa_level::none);
    assert(!g_cfg.video.precise_zpass_count.get());
    assert(!no_enabled_features.get_sample_rate_shading_support());
    assert(!no_enabled_features.get_alpha_to_one_support());
    no_enabled_features.apply_config_limits();
    assert(g_cfg.from_string(original));
    std::cout << "Real RPCS3 schema: live updates, hardware fallbacks, restart-only CPU/GPU choices and repeated refresh passed\n";
}
