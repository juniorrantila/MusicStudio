#include <audio/audio.h>
#include <synth/synth.h>
#include <stdio.h>

static int key_to_key(char c)
{
    switch (c) {
    case 'q': return SynthKey_Ab2;
    case 'w': return SynthKey_A2;
    case 'e': return SynthKey_Bb2;
    case 'r': return SynthKey_B2;
    case 't': return SynthKey_C3;
    case 'y': return SynthKey_Db3;
    case 'u': return SynthKey_D3;
    case 'i': return SynthKey_Eb3;
    case 'o': return SynthKey_E3;
    case 'p': return SynthKey_F3;
    case 'a': return SynthKey_Gb3;
    case 's': return SynthKey_G3;
    case 'd': return SynthKey_Ab3;
    case 'f': return SynthKey_A3;
    case 'g': return SynthKey_Bb3;
    case 'h': return SynthKey_B3;
    case 'j': return SynthKey_C4;
    case 'k': return SynthKey_Db4;
    case 'l': return SynthKey_D4;
    case 'z': return SynthKey_Eb4;
    case 'x': return SynthKey_E4;
    case 'c': return SynthKey_F4;
    case 'v': return SynthKey_Gb4;
    case 'b': return SynthKey_G4;
    case 'n': return SynthKey_Ab4;
    case 'm': return SynthKey_A4;
    case ',': return SynthKey_Bb4;
    case '.': return SynthKey_B4;
    }
    return -1;
}

int main(void) {
    Synth synth = (Synth) {
        .square_amplitude = 0.3,
        .square_k_value = 2.0,
        .square_harmonics = 20,

        .inner_time_warp_frequency = 0.05,
        .inner_time_warp_amplitude = 1.0,

        .outer_time_warp_frequency = 55.0,
        .outer_time_warp_amplitude = 0.015,
    };

    AudioClient* audio_client = audio_client_create("synth", synth_callback, &synth);
    if (!audio_client)
        return -1;

    int last_key = -1;
    while (true) {
        int c = getc(stdin);
        if (c == ' ') {
            if (last_key == -1)
                continue;
            if (synth_is_key_set(&synth, last_key)) {
                synth_unset_key(&synth, last_key);
            } else {
                synth_set_key(&synth, last_key);
            }
        }
        int key = key_to_key(c);
        if (key < 0)
            continue;
        if (synth_is_key_set(&synth, key)) {
            synth_unset_key(&synth, key);
        } else {
            synth_set_key(&synth, key);
        }
        last_key = key;
    }

    audio_client_destroy(audio_client);
    return 0;
}
