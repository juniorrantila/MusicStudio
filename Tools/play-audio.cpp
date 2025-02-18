#include <CLI/ArgumentParser.h>
#include <MacTypes.h>
#include <Main/Main.h>
#include <SoundIo/SoundIo.h>
#include <Ty/Optional.h>
#include <AU/Audio.h>
#include <Ty/Limits.h>
#include <Core/Print.h>
#include <Core/MappedFile.h>
#include <Core/Time.h>
#include <AU/SoundIo.h>
#include <AU/Pipeline.h>
#include <MS/VstPlugin.h>
#include <UI/Application.h>
#include <UI/Window.h>
#include <MS/WASMPluginManager.h>
#include <Ty/Swap.h>
#include <MS/Project.h>
#include <string.h>
#include <unistd.h>
#include <AU/AudioDecoder.h>
#include <Ty/SegmentedArena.h>
#include <Ty/PageAllocator.h>

struct PartTime {
    u32 hours;
    u8 minutes;
    u8 seconds;
};

static PartTime part_time(u32 seconds);

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static void underflow_callback(SoundIoOutStream *outstream);

struct Context {
    AU::Pipeline* pipeline;
    void (*write_sample)(void* ptr, f64 sample);
    usize _Atomic* played_frames;
    usize frame_count;

    View<f64> scratch;
    View<f64> scratch2;
};


ErrorOr<int> Main::main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    auto wav_path = StringView();
    TRY(argument_parser.add_positional_argument("wav-path", [&](c_string arg) {
        wav_path = StringView::from_c_string(arg);
    }));

    auto plugin_paths = Vector<StringView>();
    TRY(argument_parser.add_option("--plugin", "-p", "wasm-plugin", "pass audio through WASM plugin", [&](c_string arg) {
        MUST(plugin_paths.append(StringView::from_c_string(arg)));
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto arena_allocator = segmented_arena_create(page_allocator());
    auto* arena = &arena_allocator.allocator;

    auto wav_file = TRY(Core::MappedFile::open(wav_path));
    auto audio = TRY(AUAudioRef::decode(arena, AUFormat_WAV, wav_file.bytes()));

    auto project = MS::Project {
        .sample_rate = audio.raw.sample_rate,
        .channels = audio.raw.channel_count,
    };
    auto plugin_manager = MS::WASMPluginManager(Ref(project));
    for (auto path : plugin_paths) {
        TRY(plugin_manager.add_plugin(path));
    }
    TRY(plugin_manager.link());
    TRY(plugin_manager.init());
    Defer deinit_plugin_manager = [&] {
        plugin_manager.deinit().or_else([](Error error){
            dprintln("Error: could not deinit plugin manager: ", error);
        });
    };

    dprintln("\n----------------------");
    dprintln("Layout: {}", (u64)audio.raw.sample_layout);
    dprintln("Format: {}", (u64)audio.raw.sample_format);
    dprintln("Sample rate: {}", audio.raw.sample_rate);
    dprintln("Channels: {}", audio.raw.channel_count);
    dprintln("Duration: {}", part_time((u32)audio.duration()));
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

    _Atomic usize played_frames = 0;

    auto audio_pipeline = AU::Pipeline();
    TRY(audio_pipeline.pipe([&](f64* out, f64*, usize frames, usize channels) {
        usize played = played_frames;
        for (usize frame = 0; frame < frames; frame++) {
            for (usize channel = 0; channel < channels; channel++) {
                f64 sample = audio.sample_f64(channel, played + frame);
                out[frame * channels + channel] = sample;
            }
        }
    }));

    TRY(audio_pipeline.pipe([&](f64* const out, f64* const in, usize frames, usize channels) {
        f64* a = out;
        f64* b = in;
        for (auto& plugin : plugin_manager.plugins()) {
            MUST(plugin.process_f64(a, b, frames, channels));
            swap(&a, &b);
        }
        if (plugin_manager.plugins().size() % 2 == 0) {
            memcpy(out, in, frames * channels * sizeof(f64));
        }
    }));

    auto context = Context {
        .pipeline = &audio_pipeline,
        .write_sample = stream_writer.writer,
        .played_frames = &played_frames,
        .frame_count = audio.raw.frame_count,
        .scratch = TRY(arena->alloc<f64>(SOUNDIO_MAX_CHANNELS * (usize)audio.raw.sample_rate)),
        .scratch2 = TRY(arena->alloc<f64>(SOUNDIO_MAX_CHANNELS * (usize)audio.raw.sample_rate)),
    };

    SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        return Error::from_errno(ENOMEM);
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->sample_rate = (i32)audio.raw.sample_rate;
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

    auto duration = part_time((u32)audio.duration());
    dprint("Time: {} / {}", part_time(0), duration);
    while (played_frames < audio.raw.frame_count) {
        soundio_flush_events(soundio);
        auto current_time = part_time(played_frames / audio.raw.sample_rate);
        dprint("\r\033[KTime: {} / {}", current_time, duration);
        sleep(1);
    }
    dprintln("\r\033[KTime: {} / {}", duration, duration);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    (void)frame_count_min;
    int frames_left = frame_count_max;
    auto* ctx = (Context*)outstream->userdata;
    usize played_frames = *ctx->played_frames;

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
        f64* scratch = ctx->scratch.data();
        f64* scratch2 = ctx->scratch2.data();
        f64* out = ctx->pipeline->run(scratch, scratch2, frame_count, channel_count);
        for (int frame = 0; frame < frame_count; frame += 1, played_frames += 1) {
            for (usize channel = 0; channel < channel_count; channel += 1) {
                f64 sample = out[frame * channel_count + channel];
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

    *ctx->played_frames = played_frames;
    soundio_outstream_pause(outstream, played_frames >= ctx->frame_count);
}

static void underflow_callback(SoundIoOutStream *outstream) {
    (void)outstream;
    static usize count = 0;
    dprintln("underflow {}", count++);
}

static PartTime part_time(u32 seconds)
{
    u8 s = seconds % 60;
    u32 minutes = seconds / 60;
    u8 m = minutes % 60;
    u32 hours = minutes / 60;
    u32 h = hours;
    return {
        .hours = h,
        .minutes = m,
        .seconds = s,
    };
}

template <>
struct Ty::Formatter<PartTime> {
    template <typename U>
        requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, PartTime time)
    {
        return TRY(to.write(time.hours, "h"sv, (u32)time.minutes, "m"sv, (u32)time.seconds, "s"sv));
    }
};
