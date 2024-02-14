#include "audio.h"
#include <SoundIo/SoundIo.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct AudioClient {
    SynthCallback synth_callback;
    const void *synth_context;

    struct SoundIo* soundio;
    struct SoundIoDevice* device;
    struct SoundIoOutStream* stream;
} static audio_client_invalid = { 0 };

static void write_sample_s16ne(char *ptr, double sample) {
    int16_t *buf = (int16_t *)ptr;
    double range = (double)INT16_MAX - (double)INT16_MIN;
    double val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_s32ne(char *ptr, double sample) {
    int32_t *buf = (int32_t *)ptr;
    double range = (double)INT32_MAX - (double)INT32_MIN;
    double val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_float32ne(char *ptr, double sample) {
    float *buf = (float *)ptr;
    *buf = sample;
}

static void write_sample_float64ne(char *ptr, double sample) {
    double *buf = (double *)ptr;
    *buf = sample;
}

static void (*write_sample)(char *ptr, double sample);
static double seconds_offset = 0.0;
static volatile bool want_pause = false;
static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    AudioClient* audio_client = (AudioClient*)outstream->userdata;

    (void)frame_count_min;
    double float_sample_rate = outstream->sample_rate;
    double seconds_per_frame = 1.0 / float_sample_rate;
    struct SoundIoChannelArea *areas;
    int err;

    int frames_left = frame_count_max;

    for (;;) {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;

        const struct SoundIoChannelLayout *layout = &outstream->layout;

        for (int frame = 0; frame < frame_count; frame += 1) {
            double sample = audio_client->synth_callback(seconds_offset + frame * seconds_per_frame, audio_client->synth_context);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                write_sample(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);

        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow)
                return;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    soundio_outstream_pause(outstream, want_pause);
}

AudioClient *audio_client_create(const char *client_name, SynthCallback synth_callback, const void *synth_context) {
    AudioClient* audio_client = (AudioClient*)malloc(sizeof(AudioClient));
    if (!audio_client) {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }
    *audio_client = (AudioClient) {
        .synth_callback = synth_callback,
        .synth_context = synth_context,
    };

    struct SoundIo* soundio = soundio_create();
    if (!soundio) {
        return NULL;
    }

    int err = soundio_connect(soundio);
    if (err) {
        soundio_destroy(soundio);
        fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
        return NULL;
    }
    soundio_flush_events(soundio);

    int selected_device_index = soundio_default_output_device_index(soundio);
    if (selected_device_index < 0) {
        fprintf(stderr, "Output device not found\n");
        return NULL;
    }
    struct SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        soundio_destroy(soundio);
        fprintf(stderr, "out of memory\n");
        return NULL;
    }
    if (device->probe_error) {
        soundio_destroy(soundio);
        fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(device->probe_error));
        return NULL;
    }

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        soundio_destroy(soundio);
        fprintf(stderr, "out of memory\n");
        return NULL;
    }

    outstream->write_callback = write_callback;
    outstream->name = client_name;
    outstream->userdata = audio_client;
    if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
        outstream->format = SoundIoFormatFloat32NE;
        write_sample = write_sample_float32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
        outstream->format = SoundIoFormatFloat64NE;
        write_sample = write_sample_float64ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {
        outstream->format = SoundIoFormatS32NE;
        write_sample = write_sample_s32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
        outstream->format = SoundIoFormatS16NE;
        write_sample = write_sample_s16ne;
    } else {
        soundio_destroy(soundio);
        fprintf(stderr, "No suitable device format available.\n");
        return NULL;
    }

    if ((err = soundio_outstream_open(outstream))) {
        soundio_destroy(soundio);
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return NULL;
    }

    if (outstream->layout_error) {
        soundio_destroy(soundio);
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));
    }

    if ((err = soundio_outstream_start(outstream))) {
        soundio_destroy(soundio);
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return NULL;
    }

    return audio_client;
}

void audio_client_destroy(AudioClient *client) {
    soundio_outstream_destroy(client->stream);
    soundio_device_unref(client->device);
    soundio_destroy(client->soundio);
    *client = audio_client_invalid;
    free(client);
}

static double g_sinewave_table[0x100000];
static const unsigned g_sinewave_table_size = sizeof(g_sinewave_table) / sizeof(g_sinewave_table[0]);

__attribute__((constructor))
void initialize_sinewave_table(void)
{
    const double tau = 2 * 3.14159265359;
    for (unsigned i = 0; i < g_sinewave_table_size; i++)
        g_sinewave_table[i] = __builtin_sin(tau * (i / (double)g_sinewave_table_size));
}

double audio_sin_turns(double value)
{
    const double fraction = value - (long long)value;
    return g_sinewave_table[(unsigned long long)(fraction * (double)g_sinewave_table_size) % g_sinewave_table_size];
}
