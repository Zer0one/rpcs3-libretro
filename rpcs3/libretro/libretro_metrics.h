#pragma once

#include "libretro.h"
#include <cstdint>

// Opt-in CPU wall times and frame-flow counters. These do not measure GPU
// execution time or physical scanout. Cross-thread counters are atomic;
// frontend interval distributions (up to the latest 512 per window) belong exclusively to retro_run's thread.
namespace libretro_metrics
{
    enum class counter : unsigned
    {
        ready, shown, duplicate, dropped, steps,
        queue_wait, queue_hold, acquire, fence_wait, pause_wait, frontend_wait, handoff_submit,
        video_callback, take, audio_pull, audio_callback, audio_offered, audio_accepted,
        count
    };
    void enable(bool on);
    bool enabled();
    std::uint64_t now_us();
    void add(counter what, std::uint64_t value = 1);
    void finish_run(std::uint64_t start, retro_log_printf_t log);
    void begin_run(std::uint64_t start);

    class scope
    {
        counter m_counter;
        std::uint64_t m_start;
    public:
        explicit scope(counter what) : m_counter(what), m_start(enabled() ? now_us() : 0) {}
        ~scope() { if (m_start) add(m_counter, now_us() - m_start); }
        scope(const scope&) = delete;
        scope& operator=(const scope&) = delete;
    };
}
