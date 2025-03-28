#include <Library/HotReload.h>
#include <Ty/StringSlice.h>
#include <Ty2/Logger.h>
#include <FS/FSVolume.h>

#include <math.h>

struct Context {
    Allocator* arena;
    Logger* log;
    FSVolume* volume;
};

struct AudioContext {
    Context* host;
};

static f64 square(f64 time)
{
    time = fmod(time, 1.0);
    if (time < 0.5) return -1.0;
    return 1.0;
}

static f64 min(f64 a, f64 b)
{
    if (a < b) return a;
    return b;
}

static f64 max(f64 a, f64 b)
{
    if (a > b) return a;
    return b;
}

static f64 sample_at_time(void* ctx, f64 time)
{
    auto* audio = (AudioContext*)ctx;
    (void)audio;

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

static void init(void* state, void const* arg, usize size)
{
    VERIFY(size == sizeof(Context));
    auto* audio = (AudioContext*)state;
    audio->host = (Context*)arg;
}

static void deinit(void*)
{
}

static void* find(StringSlice symbol_name)
{
    if (symbol_name.equal("sample_at_time"s))
        return (void*)sample_at_time;
    if (symbol_name.equal("init"s))
        return (void*)init;
    if (symbol_name.equal("deinit"s))
        return (void*)deinit;
    return 0;
}

C_API void* hotreload_dispatch(HotReloadEvent event);
C_API void* hotreload_dispatch(HotReloadEvent event)
{
    switch (event.tag) {
    case HotReloadTag_Size:
        return (void*)sizeof(AudioContext);
    case HotReloadTag_Find:
        return find(string_slice((char*)event.state, event.size));
    }
}
