#pragma once
#include "Base.h"

namespace Ty {

struct Hardware {
    static Hardware the()
    {
        static Hardware hardware;

        if (static bool first = true; first) [[unlikely]] {
            first = false;
            auto info = Info::the();
            hardware.m_threads = info.threads;
            hardware.m_cores = info.cores;
        }

        return hardware;
    }

    u32 threads() const { return m_threads; }
    u32 cores() const { return m_cores; }
    static u32 current_thread()
    {
        return Info::the().current_thread;
    }

private:
    struct Info {
        u32 cores;
        u32 threads;
        u32 unknown;
        u32 current_thread;

        static Info the()
        {
            Info info;

            asm volatile("cpuid"
                         : "=a"(info.threads), "=b"(info.cores),
                         "=c"(info.unknown),
                         "=d"(info.current_thread)
                         : "0"(11), "2"(0)
                         :);

            return info;
        }
    };

    constexpr Hardware() = default;

    constexpr Hardware(u32 threads, u32 cores)
        : m_threads(threads)
        , m_cores(cores)
    {
    }

    u32 m_threads;
    u32 m_cores;
};

}

using Ty::Hardware;
