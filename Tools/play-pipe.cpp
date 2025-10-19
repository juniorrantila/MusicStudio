#include <LibCLI/ArgumentParser.h>
#include <LibMain/Main.h>
#include <LibCore/Print.h>
#include <LibAudio/SoundIo.h>
#include <LibAudio/Pipeline.h>
#include <LibTy/System.h>

#include <SoundIo/SoundIo.h>

#include <unistd.h>

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static void underflow_callback(SoundIoOutStream *outstream);

struct Context {
    AU::Pipeline* pipeline;
    void (*write_sample)(void* ptr, f64 sample);
};


ErrorOr<int> Main::main(int, c_string[]) {
    u32 channel_count = 0;
    u32 sample_rate = 0;
    TRY(System::read(0, &channel_count, sizeof(channel_count)));
    TRY(System::read(0, &sample_rate, sizeof(sample_rate)));

    dprintln("\n----------------------");
    dprintln("Channels: {}", channel_count);
    dprintln("Sample rate: {}", sample_rate);
    dprintln("----------------------\n");

    SoundIo *soundio = soundio_create();
    if (!soundio) {
        return Error::from_errno(ENOMEM);
    }

    if (int err = soundio_connect(soundio)) {
        return Error::from_string_literal(soundio_strerror(err));
    }

    soundio_flush_events(soundio);

    int selected_device_index = soundio_default_output_device_index(soundio);
    if (selected_device_index < 0) {
        return Error::from_string_literal("Output device not found");
    }

    SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        return Error::from_errno(ENOMEM);
    }
    if (device->probe_error) {
        return Error::from_string_literal(soundio_strerror(device->probe_error));
    }
    AUSoundIoWriter stream_writer = {};
    if (!au_select_soundio_writer_for_device(device, &stream_writer)) {
        return Error::from_string_literal("could find suitable stream format");
    }

    bool done = false;
    SoundIoOutStream* outstream = nullptr;
    auto audio_pipeline = AU::Pipeline();
    TRY(audio_pipeline.pipe([&](f64* out, f64*, usize frames, usize channels) {
        VERIFY(channels == channel_count);
        MUST(System::read(0, out, sizeof(f64) * channel_count * frames));
    }));

    auto context = Context {
        .pipeline = &audio_pipeline,
        .write_sample = stream_writer.writer,
    };

    outstream = soundio_outstream_create(device);
    if (!outstream) {
        return Error::from_errno(ENOMEM);
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->sample_rate = (i32)sample_rate;
    outstream->userdata = &context;
    outstream->format = stream_writer.format;

    if (int err = soundio_outstream_open(outstream)) {
        return Error::from_string_literal(soundio_strerror(err));
    }
    if (outstream->layout_error) {
        return Error::from_string_literal(soundio_strerror(outstream->layout_error));
    }
    if (int err = soundio_outstream_start(outstream)) {
        return Error::from_string_literal(soundio_strerror(err));
    }

    while (!done) {
        soundio_flush_events(soundio);
        System::sleep(1);
    }

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    (void)frame_count_min;
    int frames_left = frame_count_max;
    auto* ctx = (Context*)outstream->userdata;

    for (;;) {
        int frame_count = frames_left;
        SoundIoChannelArea* areas = nullptr;
        if (auto err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
            auto message = StringView::from_c_string(soundio_strerror(err));
            dprintln("unrecoverable stream error: {}", message);
            return;
        }

        if (!frame_count)
            break;

        SoundIoChannelLayout const* layout = &outstream->layout;
        usize channel_count = layout->channel_count;
        f64* buf = (f64*)__builtin_alloca(sizeof(f64) * frame_count * channel_count);
        ctx->pipeline->run(buf, nullptr, frame_count, channel_count);
        for (int frame = 0; frame < frame_count; frame += 1) {
            for (usize channel = 0; channel < channel_count; channel += 1) {
                f64 sample = buf[frame * channel_count + channel];
                ctx->write_sample(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }

        if (auto err = soundio_outstream_end_write(outstream)) {
            if (err == SoundIoErrorUnderflow)
                return;
            auto message = StringView::from_c_string(soundio_strerror(err));
            dprintln("unrecoverable stream error: {}", message);
            return;
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }
}

static void underflow_callback(SoundIoOutStream *outstream) {
    (void)outstream;
    static usize count = 0;
    dprintln("underflow {}", count++);
}
