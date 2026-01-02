/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include <Basic/Context.h>

#include <LibCLI/ArgumentParser.h>
#include <LibMain/Main.h>
#include <LibTy/Parse.h>

#include <SoundIo/SoundIo.h>

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static void write_sample_s16ne(char *ptr, f64 sample);
static void write_sample_s32ne(char *ptr, f64 sample);
static void write_sample_float32ne(char *ptr, f64 sample);
static void write_sample_float64ne(char *ptr, f64 sample);
static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static void underflow_callback(SoundIoOutStream *outstream);
static ErrorOr<SoundIoBackend> parse_backend(StringView name);

static volatile bool want_pause = false;
static void (*write_sample)(char *ptr, f64 sample);

namespace Main {

ErrorOr<int> main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    auto backend_name = "default"sv;
    TRY(argument_parser.add_option("--backend"sv, "-b"sv, "backend"sv, "[default, dummy, alsa, pulseaudio, jack, coreaudio, wasapi]"sv, [&](c_string arg) {
        backend_name = StringView::from_c_string(arg);
    }));

    c_string device_id = nullptr;
    TRY(argument_parser.add_option("--device"sv, "-d"sv, "device-id"sv, "device to use"sv, [&](c_string arg) {
        device_id = arg;
    }));

    c_string stream_name = nullptr;
    TRY(argument_parser.add_option("--name"sv, "-n"sv, "stream-name"sv, "stream name"sv, [&](c_string arg) {
        stream_name = arg;
    }));

    f64 latency = 0.0;
    TRY(argument_parser.add_option("--latency"sv, "-l"sv, "seconds"sv, "add latency"sv, [&](c_string arg) {
        latency = Parse<f32>::from(StringView::from_c_string(arg)).or_else([&] {
            print("Invalid value '%s' provided for '--latency'\n\n", arg);
            argument_parser.print_usage_and_exit(1);
            return 0.0f;
        });
    }));

    u32 sample_rate = 0;
    TRY(argument_parser.add_option("--sample-rate"sv, "-s"sv, "hz"sv, "sample rate"sv, [&](c_string arg) {
        sample_rate = Parse<u32>::from(StringView::from_c_string(arg)).or_else([&] {
            print("Invalid value '%s' provided for '--sample-rate'\n\n", arg);
            argument_parser.print_usage_and_exit(1);
            return 0.0f;
        });
    }));

    bool raw = false;
    TRY(argument_parser.add_flag("--raw"sv, "-r"sv, "raw mode"sv, [&] {
        raw = true;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    SoundIoBackend backend = TRY(parse_backend(backend_name));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    SoundIo *soundio = soundio_create();
    if (!soundio) {
        fatalf("out of memory");
    }

    int err = (backend == SoundIoBackendNone) ?
        soundio_connect(soundio) : soundio_connect_backend(soundio, backend);

    if (err) {
        fatalf("Unable to connect to backend: %s", soundio_strerror(err));
    }

    infof("Backend: %s", soundio_backend_name(soundio->current_backend));

    soundio_flush_events(soundio);

    int selected_device_index = -1;
    if (device_id) {
        int device_count = soundio_output_device_count(soundio);
        for (int i = 0; i < device_count; i += 1) {
            SoundIoDevice *device = soundio_get_output_device(soundio, i);
            bool select_this_one = StringView::from_c_string(device->id) == StringView::from_c_string(device_id) && device->is_raw == raw;
            soundio_device_unref(device);
            if (select_this_one) {
                selected_device_index = i;
                break;
            }
        }
    } else {
        selected_device_index = soundio_default_output_device_index(soundio);
    }

    if (selected_device_index < 0) {
        fatalf("Output device not found");
        return 1;
    }

    SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        fatalf("out of memory");
    }

    infof("Output device: %s", device->name);

    if (device->probe_error) {
        fatalf("Cannot probe device: %s", soundio_strerror(device->probe_error));
    }

    SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        fatalf("out of memory");
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = stream_name;
    outstream->software_latency = latency;
    outstream->sample_rate = sample_rate;

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
        fatalf("No suitable device format available.");
    }

    if ((err = soundio_outstream_open(outstream))) {
        fatalf("unable to open device: %s", soundio_strerror(err));
    }

    infof("Software latency: %f\n", outstream->software_latency);
    infof("'p\\n' - pause\n"
          "'u\\n' - unpause\n"
          "'P\\n' - pause from within callback\n"
          "'c\\n' - clear buffer\n"
          "'q\\n' - quit\n");

    if (outstream->layout_error)
        fatalf("unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fatalf("unable to start device: %s\n", soundio_strerror(err));
    }

    for (;;) {
        soundio_flush_events(soundio);
        int c = getc(stdin);
        if (c == 'p') {
            infof("pausing result: %s",
                    soundio_strerror(soundio_outstream_pause(outstream, true)));
        } else if (c == 'P') {
            want_pause = true;
        } else if (c == 'u') {
            want_pause = false;
            infof("unpausing result: %s",
                    soundio_strerror(soundio_outstream_pause(outstream, false)));
        } else if (c == 'c') {
            infof("clear buffer result: %s",
                    soundio_strerror(soundio_outstream_clear_buffer(outstream)));
        } else if (c == 'q') {
            break;
        } else if (c == '\r' || c == '\n') {
            // ignore
        } else {
            infof("Unrecognized command: %c\n", c);
        }
    }

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}

}

static void write_sample_s16ne(char *ptr, f64 sample) {
    i16* buf = (i16*)ptr;
    f64 range = (f64)INT16_MAX - (f64)INT16_MIN;
    f64 val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_s32ne(char *ptr, f64 sample) {
    i32* buf = (i32*)ptr;
    f64 range = (f64)INT32_MAX - (f64)INT32_MIN;
    f64 val = sample * range / 2.0;
    *buf = val;
}

static void write_sample_float32ne(char *ptr, f64 sample) {
    f32* buf = (f32*)ptr;
    *buf = sample;
}

static void write_sample_float64ne(char *ptr, f64 sample) {
    f64* buf = (f64*)ptr;
    *buf = sample;
}

static const f64 PI = 3.14159265358979323846264338328;
static f64 seconds_offset = 0.0;
static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    (void)frame_count_min;
    f64 float_sample_rate = outstream->sample_rate;
    f64 seconds_per_frame = 1.0 / float_sample_rate;
    SoundIoChannelArea *areas;
    int err;

    int frames_left = frame_count_max;

    for (;;) {
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fatalf("unrecoverable stream error: %s\n", soundio_strerror(err));
        }

        if (!frame_count)
            break;

        const SoundIoChannelLayout *layout = &outstream->layout;

        f64 pitch = 440.0;
        f64 radians_per_second = pitch * 2.0 * PI;
        for (int frame = 0; frame < frame_count; frame += 1) {
            f64 sample = sin((seconds_offset + frame * seconds_per_frame) * radians_per_second);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                write_sample(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);

        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow)
                return;
            fatalf("unrecoverable stream error: %s\n", soundio_strerror(err));
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    soundio_outstream_pause(outstream, want_pause);
}

static void underflow_callback(SoundIoOutStream *outstream) {
    (void)outstream;
    static int count = 0;
    errorf("underflow %d\n", count++);
}

static ErrorOr<SoundIoBackend> parse_backend(StringView name)
{
    if (name == "default"sv)
        return SoundIoBackendNone;
    if (name == "dummy"sv)
        return SoundIoBackendDummy;
    if (name == "alsa"sv)
        return SoundIoBackendAlsa;
    if (name == "pulseaudio"sv)
        return SoundIoBackendPulseAudio;
    if (name == "jack"sv)
        return SoundIoBackendJack;
    if (name == "coreaudio"sv)
        return SoundIoBackendCoreAudio;
    if (name == "wasapi"sv)
        return SoundIoBackendWasapi;
    return Error::from_string_literal("invalid backend");
}
