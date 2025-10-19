/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include <LibCLI/ArgumentParser.h>
#include <LibMain/Main.h>
#include <SoundIo/SoundIo.h>

#include <stdio.h>

static void on_devices_change(SoundIo *soundio);
static int list_devices(SoundIo *soundio);
static void print_device(SoundIoDevice *device, bool is_default);
static void print_channel_layout(const SoundIoChannelLayout *layout);
static ErrorOr<SoundIoBackend> parse_backend(StringView name);

static bool short_output = false;

namespace Main {

ErrorOr<int> main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    bool watch = false;
    TRY(argument_parser.add_flag("--watch"sv, "-w"sv, "enter watch mode", [&] {
        watch = true;
    }));

    TRY(argument_parser.add_flag("--short"sv, "-s"sv, "short output", [&] {
        short_output = true;
    }));

    auto backend_name = "default"sv;
    TRY(argument_parser.add_option("--backend"sv, "-b"sv, "backend"sv, "[default, dummy, alsa, pulseaudio, jack, coreaudio, wasapi]"sv, [&](c_string arg) {
        backend_name = StringView::from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    SoundIoBackend backend = TRY(parse_backend(backend_name));

    SoundIo *soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    int err = (backend == SoundIoBackendNone) ?
        soundio_connect(soundio) : soundio_connect_backend(soundio, backend);

    if (err) {
        fprintf(stderr, "%s\n", soundio_strerror(err));
        return err;
    }

    if (watch) {
        soundio->on_devices_change = on_devices_change;
        for (;;) {
            soundio_wait_events(soundio);
        }
    } else {
        soundio_flush_events(soundio);
        int err = list_devices(soundio);
        soundio_destroy(soundio);
        return err;
    }
}

}

static void print_channel_layout(const SoundIoChannelLayout *layout)
{
    if (layout->name) {
        fprintf(stderr, "%s", layout->name);
    } else {
        fprintf(stderr, "%s", soundio_get_channel_name(layout->channels[0]));
        for (int i = 1; i < layout->channel_count; i += 1) {
            fprintf(stderr, ", %s", soundio_get_channel_name(layout->channels[i]));
        }
    }
}

static void print_device(SoundIoDevice *device, bool is_default)
{
    const char *default_str = is_default ? " (default)" : "";
    const char *raw_str = device->is_raw ? " (raw)" : "";
    fprintf(stderr, "%s%s%s\n", device->name, default_str, raw_str);
    if (short_output)
        return;
    fprintf(stderr, "  id: %s\n", device->id);

    if (device->probe_error) {
        fprintf(stderr, "  probe error: %s\n", soundio_strerror(device->probe_error));
    } else {
        fprintf(stderr, "  channel layouts:\n");
        for (int i = 0; i < device->layout_count; i += 1) {
            fprintf(stderr, "    ");
            print_channel_layout(&device->layouts[i]);
            fprintf(stderr, "\n");
        }
        if (device->current_layout.channel_count > 0) {
            fprintf(stderr, "  current layout: ");
            print_channel_layout(&device->current_layout);
            fprintf(stderr, "\n");
        }

        fprintf(stderr, "  sample rates:\n");
        for (int i = 0; i < device->sample_rate_count; i += 1) {
            SoundIoSampleRateRange *range = &device->sample_rates[i];
            fprintf(stderr, "    %d - %d\n", range->min, range->max);

        }
        if (device->sample_rate_current)
            fprintf(stderr, "  current sample rate: %d\n", device->sample_rate_current);
        fprintf(stderr, "  formats: ");
        for (int i = 0; i < device->format_count; i += 1) {
            const char *comma = (i == device->format_count - 1) ? "" : ", ";
            fprintf(stderr, "%s%s", soundio_format_string(device->formats[i]), comma);
        }
        fprintf(stderr, "\n");
        if (device->current_format != SoundIoFormatInvalid)
            fprintf(stderr, "  current format: %s\n", soundio_format_string(device->current_format));

        fprintf(stderr, "  min software latency: %0.8f sec\n", device->software_latency_min);
        fprintf(stderr, "  max software latency: %0.8f sec\n", device->software_latency_max);
        if (device->software_latency_current != 0.0)
            fprintf(stderr, "  current software latency: %0.8f sec\n", device->software_latency_current);

    }
    fprintf(stderr, "\n");
}

static int list_devices(SoundIo *soundio)
{
    int output_count = soundio_output_device_count(soundio);
    int input_count = soundio_input_device_count(soundio);

    int default_output = soundio_default_output_device_index(soundio);
    int default_input = soundio_default_input_device_index(soundio);

    fprintf(stderr, "--------Input Devices--------\n\n");
    for (int i = 0; i < input_count; i += 1) {
        SoundIoDevice *device = soundio_get_input_device(soundio, i);
        print_device(device, default_input == i);
        soundio_device_unref(device);
    }
    fprintf(stderr, "\n--------Output Devices--------\n\n");
    for (int i = 0; i < output_count; i += 1) {
        SoundIoDevice *device = soundio_get_output_device(soundio, i);
        print_device(device, default_output == i);
        soundio_device_unref(device);
    }

    fprintf(stderr, "\n%d devices found\n", input_count + output_count);
    return 0;
}

static void on_devices_change(SoundIo *soundio)
{
    fprintf(stderr, "devices changed\n");
    list_devices(soundio);
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
