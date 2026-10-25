#include "stdafx.h"
#include "libretro_metrics.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>

LOG_CHANNEL(lr_timing_log, "LibretroTiming");

namespace libretro_metrics
{
    namespace
    {
        constexpr unsigned count = static_cast<unsigned>(counter::count);
        std::atomic<bool> active{false};
        std::array<std::atomic<std::uint64_t>, count> sums{}, maxima{};
        struct series
        {
            std::array<std::uint64_t, 512> values{};
            unsigned n = 0, total = 0;
            std::uint64_t sum = 0;
            void push(std::uint64_t v)
            {
                sum += v;
                values[total++ % values.size()] = v;
                n = std::min<unsigned>(total, values.size());
            }
            double p95() const
            {
                if (!n) return 0;
                auto sorted = values;
                std::sort(sorted.begin(), sorted.begin() + n);
                return sorted[(95 * n + 99) / 100 - 1] / 1000.;
            }
        };
        // Only enable/begin_run/finish_run on the frontend thread touch these.
        std::uint64_t window_start = 0, previous_start = 0, previous_end = 0;
        unsigned runs = 0;
        series periods, outside, execution;
    }

    bool enabled() { return active.load(std::memory_order_relaxed); }
    std::uint64_t now_us()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    void add(counter what, std::uint64_t value)
    {
        if (!enabled()) return;
        const auto index = static_cast<unsigned>(what);
        sums[index].fetch_add(value, std::memory_order_relaxed);
        auto old = maxima[index].load(std::memory_order_relaxed);
        while (old < value && !maxima[index].compare_exchange_weak(old, value, std::memory_order_relaxed)) {}
    }
    void enable(bool on)
    {
        if (active.exchange(on, std::memory_order_relaxed) == on) return;
        for (auto& v : sums) v.store(0, std::memory_order_relaxed);
        for (auto& v : maxima) v.store(0, std::memory_order_relaxed);
        window_start = previous_start = previous_end = 0;
        runs = 0; periods = {}; outside = {}; execution = {};
    }
    void begin_run(std::uint64_t start)
    {
        if (!enabled()) return;
        if (!window_start) window_start = start;
        if (previous_start) periods.push(start - previous_start);
        if (previous_end) outside.push(start - previous_end);
        previous_start = start;
    }
    void finish_run(std::uint64_t start, retro_log_printf_t log)
    {
        if (!start || !enabled()) return;
        const auto end = now_us();
        execution.push(end - start); ++runs;
        previous_end = end;
        if (end - window_start < 1'000'000) return;
        std::array<std::uint64_t, count> values{}, peaks{};
        for (unsigned i = 0; i < count; ++i)
        {
            values[i] = sums[i].exchange(0, std::memory_order_relaxed);
            peaks[i] = maxima[i].exchange(0, std::memory_order_relaxed);
        }
        const auto val = [&](counter c) { return values[static_cast<unsigned>(c)]; };
        const auto ms = [&](counter c) { return val(c) / 1000.; };
        const auto peak = [&](counter c) { return peaks[static_cast<unsigned>(c)] / 1000.; };
        const double seconds = (end - window_start) / 1e6;
        char line[2048];
        std::snprintf(line, sizeof(line),
            "[RPCS3 Timing] host_us=%llu window_s=%.6f runs=%u run_hz=%.3f vk_ready_hz=%.3f shown_hz=%.3f "
            "dup=%llu dropped=%llu steps=%llu interval_samples=%u run_samples=%u run_ms=%.3f run_p95_ms=%.3f period_p95_ms=%.3f outside_ms=%.3f "
            "audio_pull_ms=%.3f audio_cb_ms=%.3f audio_offered=%llu audio_accepted=%llu "
            "video_cb_ms=%.3f take_ms=%.3f acquire_ms=%.3f acquire_max_ms=%.3f "
            "queue_wait_ms=%.3f queue_wait_max_ms=%.3f queue_hold_ms=%.3f fence_wait_ms=%.3f frontend_wait_ms=%.3f handoff_submit_ms=%.3f pause_wait_ms=%.3f",
            static_cast<unsigned long long>(end), seconds, runs, runs / seconds, val(counter::ready) / seconds, val(counter::shown) / seconds,
            static_cast<unsigned long long>(val(counter::duplicate)), static_cast<unsigned long long>(val(counter::dropped)),
            static_cast<unsigned long long>(val(counter::steps)), periods.n, execution.n, execution.sum / (1000. * runs), execution.p95(), periods.p95(),
            outside.sum / (1000. * std::max(1u, outside.total)), ms(counter::audio_pull), ms(counter::audio_callback),
            static_cast<unsigned long long>(val(counter::audio_offered)), static_cast<unsigned long long>(val(counter::audio_accepted)),
            ms(counter::video_callback), ms(counter::take), ms(counter::acquire), peak(counter::acquire),
            ms(counter::queue_wait), peak(counter::queue_wait), ms(counter::queue_hold), ms(counter::fence_wait), ms(counter::frontend_wait), ms(counter::handoff_submit), ms(counter::pause_wait));
        if (log) log(RETRO_LOG_INFO, "%s\n", line);
        lr_timing_log.notice("%s", line);
        // Logging is outside the next measured run; do not count it as
        // frontend work. Keep the preceding start for actual call cadence.
        previous_end = now_us();
        window_start = previous_end;
        runs = 0; periods = {}; outside = {}; execution = {};
    }
}
