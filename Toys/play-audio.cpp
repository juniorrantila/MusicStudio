#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <SoundIo/SoundIo.h>
#include <Ty/Optional.h>
#include <AU/Audio.h>
#include <Ty/Limits.h>
#include <Core/Print.h>
#include <Core/MappedFile.h>
#include <Core/Time.h>
#include <AU/SoundIo.h>

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static void underflow_callback(SoundIoOutStream *outstream);

struct Context {
    AU::Audio* audio;
    void (*write_sample)(void* ptr, f64 sample);
    _Atomic usize played_frames;
};


ErrorOr<int> Main::main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    auto wav_path = StringView();
    TRY(argument_parser.add_positional_argument("wav-path", [&](c_string arg) {
        wav_path = StringView::from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto wav_file = TRY(Core::MappedFile::open(wav_path));
    auto audio = TRY(AU::Audio::decode(AU::AudioFormat::WAV, wav_file.bytes()));

    dprintln("\n----------------------");
    dprintln("Sample rate: {}", audio.sample_rate());
    dprintln("Channels: {}", audio.channel_count());
    dprintln("Duration: {}", (usize)audio.duration());
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
    auto stream_writer = TRY(AU::select_writer_for_device(device).or_throw([]{
        return Error::from_string_literal("could find suitable stream format");
    }));
    auto context = Context {
        .audio = &audio,
        .write_sample = stream_writer.writer,
        .played_frames = 0ULL,
    };

    SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        return Error::from_errno(ENOMEM);
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->sample_rate = (i32)audio.sample_rate();
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

    auto last = Core::time();
    dprint("Time: {} / {}", 0, (usize)audio.duration());
    while (context.played_frames < audio.frame_count()) {
        usize played_frames = context.played_frames;
        soundio_flush_events(soundio);
        auto current_time = (usize)((f64)played_frames / (f64)audio.sample_rate());
        auto now = Core::time();
        if (now >= last + 0.5) {
            last = now;
            dprint("\r\033[KTime: {} / {}", current_time, (usize)audio.duration());
        }
    }
    dprintln("\r\033[KTime: {} / {}", (usize)audio.duration(), (usize)audio.duration());

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    (void)frame_count_min;
    int frames_left = frame_count_max;
    auto* ctx = (Context*)outstream->userdata;
    usize played_frames = ctx->played_frames;
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

        const SoundIoChannelLayout *layout = &outstream->layout;
        for (int frame = 0; frame < frame_count; frame += 1, played_frames += 1) {
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                f64 sample = ctx->audio->sample(played_frames, channel).or_else(0);
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

    ctx->played_frames = played_frames; 
    soundio_outstream_pause(outstream, played_frames >= ctx->audio->frame_count());
}

static void underflow_callback(SoundIoOutStream *outstream) {
    (void)outstream;
    static usize count = 0;
    dprintln("underflow {}", count++);
}
