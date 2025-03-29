#pragma once
#include "Ty/Traits.h"
#include <Ty/Base.h>

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

C_API f64 core_time_now();
C_API f64 core_time_since_start();
