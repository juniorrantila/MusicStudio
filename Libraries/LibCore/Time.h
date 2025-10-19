#pragma once
#include <LibTy/Traits.h>
#include <LibTy/Base.h>

#ifdef __cplusplus
#include <time.h>

namespace Core {

f64 time();

template <typename T>
struct Benchmark {
    T result;
    f64 time;
};

template <typename F>
f64 benchmark(F callback)
    requires Ty::IsCallableWithArguments<F, void>
{
    f64 start = time();
    callback();
    return time() - start;
}

template <typename F>
auto benchmark(F callback)
    requires (!Ty::IsCallableWithArguments<F, void>)
{
    f64 start = time();
    return Benchmark {
        .result = callback(),
        .time = time() - start,
    };
}

}

static consteval struct timespec operator ""_ms(unsigned long long value)
{
    long long v = (long long)value;
    struct timespec time = {
        .tv_sec = 0,
        .tv_nsec = v * 1000000,
    };
    while (v >= 1000) {
        time.tv_sec += 1;
        v /= 1000;
    }
    time.tv_nsec = v * 1000000LL;
    return (struct timespec){
        .tv_sec = 0,
        .tv_nsec = v * 1000000,
    };
}

#endif

C_API f64 core_time_now();
C_API f64 core_time_since_unspecified_epoch();
