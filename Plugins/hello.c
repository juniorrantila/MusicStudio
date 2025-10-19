#include <MS/WASMPlugin/Plugin.h>
#include <LibMath/Math.h>

u32 const ms_plugin_version = 1;
char const ms_plugin_id[] = "com.music-studio.hello";
char const ms_plugin_name[] = "Hello";
const MSPluginKind ms_plugin_kind = MSPluginKind_Generator;

typedef enum {
    WaveKind_Sine,
    WaveKind_Triangle,
    WaveKind_Square,
    WaveKind__Count,
    WaveKind__Min = WaveKind_Sine,
    WaveKind__Max = WaveKind_Square,
} WaveKind;

static struct {
    char const* name;
    u32 size;
} wave_kind_name[WaveKind__Count] = {
    [WaveKind_Sine] = {
        .name = "sine",
        .size = sizeof("sine") - 1,
    },
    [WaveKind_Triangle] = {
        .name = "triangle",
        .size = sizeof("triangle") - 1,
    },
    [WaveKind_Square] = {
        .name = "square",
        .size = sizeof("square") - 1,
    },
};

typedef enum {
    Parameter_Pitch,
    Parameter_WaveKind,
    Parameter__Count,
} Parameter;

static struct {
    f64 seconds_offset;
    f64 seconds_per_frame;
    f64 pitch;
    WaveKind wave_kind;
} state;

static struct {
    MSPluginParameterKind kind;
    char const* name;
    u32 name_size;
    f64 min;
    f64 max;
    f64 step_size;
} parameters[Parameter__Count] = {
    [Parameter_Pitch] = {
        .kind = MSPluginParameterKind_Knob,
        .name = "pitch",
        .name_size = sizeof("pitch") - 1,
        .min = 20.0,
        .max = 2000.0,
        .step_size = 10,
    },
    [Parameter_WaveKind] = {
        .kind = MSPluginParameterKind_Options,
        .name = "wave kind",
        .name_size = sizeof("wave kind") - 1,
        .min = WaveKind__Min,
        .max = WaveKind__Max,
        .step_size = 1.0,
    },
};

static f64 wave(WaveKind kind, f64 t);

void ms_plugin_process_f64(f64* out, f64 const* in, u32 frames, u32 channels)
{
    (void)in;
    for (u32 frame = 0; frame < frames; frame++) {
        f64 t = state.seconds_offset + frame * state.seconds_per_frame;
        f64 sample = wave(state.wave_kind, t * state.pitch);
        for (u32 channel = 0; channel < channels; channel++) {
            u32 index = frame * channels + channel;
            out[index] = sample;
        }
    }
    f64 t = state.seconds_per_frame * frames;
    state.seconds_offset = math_mod_f64(state.seconds_offset + t, 1.0);
}

static f64 wave(WaveKind kind, f64 t)
{
    switch (kind) {
    case WaveKind__Count:
    case WaveKind_Sine:
        return math_sin_turns_f64(t);
    case WaveKind_Triangle:
        return math_abs_f64(t);
    case WaveKind_Square:
        return math_round_f64(t);
    }
    return math_sin_turns_f64(t);
}

void ms_plugin_init(u32 version)
{
    (void)version;
    state.seconds_offset = 0;
    state.seconds_per_frame = 1 / (f64)ms_get_sample_rate();
    state.pitch = 440;
    state.wave_kind = WaveKind_Sine;
}

void ms_plugin_deinit(void) {}

u32 ms_plugin_parameter_count(void)
{
    return Parameter__Count;
}

f64 ms_plugin_get_parameter(u32 id)
{
    switch ((Parameter)id) {
    case Parameter_Pitch:    return state.pitch;
    case Parameter_WaveKind: return state.wave_kind;
    case Parameter__Count: break;
    }
    ms_fatal("unknown parameter %d", id);
}

void ms_plugin_set_parameter(u32 id, f64 value)
{
    switch ((Parameter)id) {
    case Parameter_Pitch:    state.pitch     = value; break;
    case Parameter_WaveKind: state.wave_kind = value; break;
    case Parameter__Count: break;
    }
}

MSPluginParameterKind ms_plugin_parameter_kind(u32 id)
{
    if (id >= Parameter__Count) {
        ms_fatal("unknown parameter id %d", id);
        return 0;
    }
    return parameters[id].kind;
}

char const* ms_plugin_parameter_name(u32 id, u32* size)
{
    if (id >= Parameter__Count) {
        ms_fatal("unknown parameter id %d", id);
        return 0;
    }
    *size = parameters[id].name_size;
    return parameters[id].name;
}

f64 ms_plugin_parameter_min_value(u32 id)
{
    if (id >= Parameter__Count) {
        ms_fatal("unknown parameter id %d", id);
        return 0;
    }
    return parameters[id].min;
}

f64 ms_plugin_parameter_max_value(u32 id)
{
    if (id >= Parameter__Count) {
        ms_fatal("unknown parameter id %d", id);
        return 0;
    }
    return parameters[id].max;
}

f64 ms_plugin_parameter_step_size(u32 id)
{
    if (id >= Parameter__Count) {
        ms_fatal("unknown parameter id %d", id);
        return 0;
    }
    return parameters[id].step_size;
}

char const* ms_plugin_parameter_option_name(u32 parameter_id, u32 option_id, u32* size)
{
    if (parameter_id != Parameter_WaveKind) {
        ms_fatal("parameter is not an option parameter: %d", parameter_id);
        return 0;
    }
    if (option_id < WaveKind__Min || option_id > WaveKind__Max) {
        ms_fatal("invalid option %d for parameter %d", option_id, parameter_id);
        return 0;
    }
    *size = wave_kind_name[option_id].size;
    return wave_kind_name[option_id].name;
}
