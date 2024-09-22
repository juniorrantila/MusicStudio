#include "synth.h"
#include "synthkey.h"

#include <audio/audio.h>
#include <math.h>
#include <stdlib.h>

static double key_to_frequency[SynthKey__Size];

static double square_oscillator(double time, double frequency, double amplitude, double k_value, int num_harmonics);
static double sine_oscillator(double time, double frequency, double amplitude);

static double voice(Synth synth, double time, double main_frequency)
{
    return square_oscillator(
        (
            time + sine_oscillator(
                time + sine_oscillator(time, synth.inner_time_warp_frequency, synth.inner_time_warp_amplitude),
                synth.outer_time_warp_frequency,
                synth.outer_time_warp_amplitude
            )
        ),
        main_frequency,
        synth.square_amplitude,
        synth.square_k_value,
        synth.square_harmonics
    );
}

double synth_callback(double time, const void* context) {
    const Synth synth = *(const Synth *)context;
    double out = 0.0;
    for (uint8_t key = 0; key < SynthKey__Size; key++) {
        if (!synth_is_key_set(&synth, (SynthKey)key))
            continue;
        out += voice(synth, time, key_to_frequency[key]);
    }
    return out;
}

static double sine_oscillator(double time, double frequency, double amplitude) {
    return audio_sin_turns(frequency * time) * amplitude;
}

static double square_oscillator(
    double time,
    double frequency,
    double amplitude,
    double k_value,
    int num_harmonics
) {
    double square = 0.0;
    double k = 1.0;
    for (int i = 0; i < num_harmonics; i++, k += k_value) {
        square += 1.0 / k * sine_oscillator(time, k * frequency, amplitude);
    }

    return square;
}

static double key_to_frequency[SynthKey__Size] =
{
    65.40639, // SynthKey_C2,
    69.29566, // SynthKey_Db2,
    73.41619, // SynthKey_D2,
    77.78175, // SynthKey_Eb2,
    82.40689, // SynthKey_E2,
    87.30706, // SynthKey_F2,
    92.49861, // SynthKey_Gb2,
    97.99886, // SynthKey_G2,
    103.8262, // SynthKey_Ab2,
    110.0000, // SynthKey_A2,
    116.5409, // SynthKey_Bb2,
    123.4708, // SynthKey_B2,

    130.8128, // SynthKey_C3,
    138.5913, // SynthKey_Db3,
    146.8324, // SynthKey_D3,
    155.5635, // SynthKey_Eb3,
    164.8138, // SynthKey_E3,
    174.6141, // SynthKey_F3,
    184.9972, // SynthKey_Gb3,
    195.9977, // SynthKey_G3,
    207.6523, // SynthKey_Ab3,
    220.0000, // SynthKey_A3,
    233.0819, // SynthKey_Bb3,
    246.9417, // SynthKey_B3,

    261.6256, // SynthKey_C4,
    277.1826, // SynthKey_Db4,
    293.6648, // SynthKey_D4,
    311.1270, // SynthKey_Eb4,
    329.6276, // SynthKey_E4,
    349.2282, // SynthKey_F4,
    369.9944, // SynthKey_Gb4,
    391.9954, // SynthKey_G4,
    415.3047, // SynthKey_Ab4,
    440.0000, // SynthKey_A4,
    466.1638, // SynthKey_Bb4,
    493.8833, // SynthKey_B4,

    523.2511, // SynthKey_C5,
    554.3653, // SynthKey_Db5,
    587.3295, // SynthKey_D5,
    622.2540, // SynthKey_Eb5,
    659.2551, // SynthKey_E5,
    698.4565, // SynthKey_F5,
    739.9888, // SynthKey_Gb5,
    783.9909, // SynthKey_G5,
    830.6094, // SynthKey_Ab5,
    880.0000, // SynthKey_A5,
    932.3275, // SynthKey_Bb5,
    987.7666, // SynthKey_B5,
};
