#pragma once

typedef struct AudioClient AudioClient;
typedef double (*SynthCallback)(double time, const void *context);

AudioClient *audio_client_create(const char *client_name, SynthCallback synth_callback, const void *synth_context);
void audio_client_destroy(AudioClient *);

double audio_sin_turns(double);
