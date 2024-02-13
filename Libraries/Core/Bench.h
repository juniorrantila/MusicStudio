#pragma once
#include <Core/File.h>
#include <Ty/Base.h>
#include <Ty/Defer.h>

namespace Core {

enum class BenchEnableAutoDisplay : bool {
    No = false,
    Yes = true,
};

struct Bench {

    constexpr Bench(BenchEnableAutoDisplay display,
        Core::File& output_file = Core::File::stderr())
        : m_out(output_file)
        , m_should_show_on_stop(
              display == BenchEnableAutoDisplay::Yes)
    {
    }

    static u64 current_tick() { return __rdtsc(); }

    void start()
    {
        m_total_cycles += m_stop_cycle - m_start_cycle;
        m_start_cycle = current_tick();
    }

    void stop() { m_stop_cycle = current_tick(); }

    template <typename Callback>
    decltype(auto) operator()(StringView message, Callback callback)
    {
        start();
        Defer on_scope_end = [message, this] {
            stop();
            if (m_should_show_on_stop)
                show(message);
        };
        return callback();
    }

    void show(StringView message) const
    {
        auto cycles = m_stop_cycle - m_start_cycle;
        auto total = m_total_cycles + cycles;

        auto const* cycles_postfix = "";
        if (cycles > 1000) {
            cycles /= 1000;
            cycles_postfix = "K";
        }
        if (cycles > 1000) {
            cycles /= 1000;
            cycles_postfix = "M";
        }

        auto const* total_postfix = "";
        if (total > 1000) {
            total /= 1000;
            total_postfix = "K";
        }
        if (total > 1000) {
            total /= 1000;
            total_postfix = "M";
        }

        auto buffer = StringBuffer();
        auto bytes = __builtin_snprintf(buffer.mutable_data(),
            buffer.capacity(),
            "%12.*s: %4d%s cycles | total: %d%s\n", message.size,
            message.data, (u32)cycles, cycles_postfix, (u32)total,
            total_postfix);
        auto view = StringView { buffer.data(), (u32)bytes };
        m_out.write(view).ignore();
    }

    constexpr u64 start_cycle() const { return m_start_cycle; }
    constexpr u64 stop_cycle() const { return m_stop_cycle; }
    constexpr u64 total_cycles() const { return m_total_cycles; }

private:
    Core::File& m_out;
    u64 m_start_cycle { 0 };
    u64 m_stop_cycle { 0 };
    u64 m_total_cycles { 0 };
    bool m_should_show_on_stop { false };
};

}
