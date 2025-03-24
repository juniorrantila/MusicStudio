#include <Library/HotReload.h>

#include <math.h>

C_API f64 square(f64 time);
C_API f64 square(f64 time)
{
    time = fmod(time, 1.0);
    if (time < 0.5) return -1.0;
    return 1.0;
}

C_API f64 min(f64 a, f64 b);
C_API f64 min(f64 a, f64 b)
{
    if (a < b) return a;
    return b;
}

C_API f64 max(f64 a, f64 b);
C_API f64 max(f64 a, f64 b)
{
    if (a > b) return a;
    return b;
}

C_API f64 sample_at_time(void*, f64 time);
C_API f64 sample_at_time(void*, f64 time)
{
    time *= 0.4;
    (void)min;
    f64 m = sin(time * 2 * M_PI * 100) * 0.5;
    f64 cm = sin(time * 1.5);
    f64 fade = fmod(time, 1.0) * 1.2;
    f64 wobble = fmod(time * 0.2, 1.0) * 0.3;

    f64 a = fade * square(m * time * 240.0) * 0.1;
    f64 b = square(cm * time * 100.0) * 0.2;
    a = max(a, b);
    b = sin(wobble * m * time * 2.0 * M_PI * 820.0) * 0.2;
    return max(a, b);
}

C_API void* hotreload_dispatch(HotReloadEvent event);
C_API void* hotreload_dispatch(HotReloadEvent event)
{
    switch (event.tag) {
    case HotReloadTag_Size:
    case HotReloadTag_Init:
    case HotReloadTag_Deinit:
        return 0;
    }
}
