#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "synthkey.h"

typedef struct {
    _Atomic double square_amplitude;
    _Atomic double square_k_value;
    _Atomic int square_harmonics;

    _Atomic double inner_time_warp_frequency;
    _Atomic double inner_time_warp_amplitude;

    _Atomic double outer_time_warp_frequency;
    _Atomic double outer_time_warp_amplitude;

    _Atomic uint8_t synth_keys[SynthKey__Size / 8 + 1];
} Synth;

double synth_callback(double time, const void* synth_context);

static inline void synth_set_key(Synth* synth, SynthKey key)
{
    synth->synth_keys[key / 8] |= 1 << (key % 8);
}

static inline void synth_unset_key(Synth* synth, SynthKey key)
{
    synth->synth_keys[key / 8] &= ~(1 << (key % 8));
}

static inline bool synth_is_key_set(const Synth* synth, SynthKey key)
{
    return (synth->synth_keys[key / 8 ] & 1 << (key % 8)) > 0;
}
