#include <AU/AudioDecoder.h>
#include <AU/Pipeline.h>
#include <AU/SoundIo.h>
#include <AU/Transcoder.h>
#include <CLI/ArgumentParser.h>
#include <Core/MappedFile.h>
#include <Core/Print.h>
#include <Core/Time.h>
#include <MS/Project.h>
#include <MS/VstPlugin.h>
#include <MS/WASMPluginManager.h>
#include <MacTypes.h>
#include <Main/Main.h>
#include <SoundIo/SoundIo.h>
#include <Ty/Limits.h>
#include <Ty/Optional.h>
#include <Ty/Swap.h>
#include <Ty2/FixedArena.h>
#include <Ty2/PageAllocator.h>
#include <UI/Application.h>
#include <UI/Window.h>
#include <Vst/Rectangle.h>

#include <string.h>
#include <unistd.h>

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

static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_audio(FixedArena* arena, MS::Plugin* plugin);

ErrorOr<int> Main::main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    c_string wav_path = nullptr;
    TRY(argument_parser.add_positional_argument("wav-path", [&](c_string arg) {
        wav_path = arg;
    }));

    auto wasm_plugin_paths = Vector<StringView>();
    TRY(argument_parser.add_option("--wasm-plugin", "-wp", "wasm-plugin", "pass audio through WASM plugin", [&](c_string arg) {
        MUST(wasm_plugin_paths.append(StringView::from_c_string(arg)));
    }));

    auto vst2_plugin_paths = Vector<c_string>();
    TRY(argument_parser.add_option("--vst2-plugin", "-v2p", "vst2-plugin", "pass audio through VST2 plugin", [&](c_string arg) {
        MUST(vst2_plugin_paths.append(arg));
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    constexpr u64 arena_size = 2LLU * 1024LLU * 1024LLU * 1024LLU;
    auto arena_allocator = fixed_arena_init(page_alloc(arena_size), arena_size);
    auto* arena = &arena_allocator.allocator;

    constexpr u64 pipe_arena_size = 1024LLU * 1024LLU * 1024LLU;
    auto pipe_arena = fixed_arena_init(page_alloc(pipe_arena_size), pipe_arena_size);

    auto wav_file = TRY(Core::MappedFile::open(wav_path));

    AUAudio audio;
    auto err = au_audio_decode_wav(wav_file.bytes(), &audio);
    if (err != e_au_decode_none)
        return Error::from_string_literal(au_decode_strerror(err));

    auto project = MS::Project {
        .sample_rate = audio.sample_rate,
        .channels = audio.channel_count,
    };
    auto wasm_plugin_manager = MS::WASMPluginManager(&project);
    for (auto path : wasm_plugin_paths) {
        TRY(wasm_plugin_manager.add_plugin(path));
    }
    TRY(wasm_plugin_manager.link());
    TRY(wasm_plugin_manager.init());
    Defer deinit_plugin_manager = [&] {
        wasm_plugin_manager.deinit().or_else([](Error error){
            dprintln("Error: could not deinit plugin manager: ", error);
        });
    };

    auto vst2_plugins = Vector<MS::Plugin>();
    for (c_string path : vst2_plugin_paths) {
        TRY(vst2_plugins.append(TRY(MS::Plugin::create_from(path))));
    }

    auto* app = ui_application_create(0);
    if (!app) return Error::from_string_literal("could not create app");

    auto* root_window = ui_window_create(app, {
        .parent = nullptr,
        .title = "play audio",
        .x = 0,
        .y = 0,
        .width = 0,
        .height = 0,
    });
    if (!root_window) return Error::from_string_literal("could not create root window");
    for (auto& plugin : vst2_plugins) {
        if (!plugin.has_editor()) continue;
        auto rect = plugin.editor_rectangle().or_default({
            .y = 0,
            .x = 0,
            .height = 600,
            .width = 800
        });
        auto plugin_name = plugin.name().or_default("VST2 Plugin");
        auto* name = (char*)memclone_zero_extend(arena, plugin_name.data(), plugin_name.size(), 1, 1);
        UIWindow* window = ui_window_create(app, {
            .parent = root_window,
            .title = name,
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = rect.height,
        });
        (void)plugin.vst->set_process_precision(Vst::Precision::F64);
        ui_window_set_resizable(window, false);
        plugin.on_editor_resize = [=](i32 width, i32 height) {
            ui_window_set_size(window, vec2f((float)width, (float)height));
        };
        if (!plugin.open_editor(ui_window_native_handle(window))) {
            dprintln("could not open editor for {}", plugin_name);
        }
    }

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

    SoundIoDevice* device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        return Error::from_errno(ENOMEM);
    }
    if (device->probe_error) {
        return Error::from_string_literal(soundio_strerror(device->probe_error));
    }
    auto stream_writer = TRY(AU::select_writer_for_device(device).or_throw([]{
        return Error::from_string_literal("could find suitable stream format");
    }));

    auto audio_pipeline = AU::Pipeline();

    _Atomic usize played_frames = 0;

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
        for (auto& plugin : wasm_plugin_manager.plugins()) {
            MUST(plugin.process_f64(a, b, frames, channels));
            swap(&a, &b);
        }
        if (wasm_plugin_manager.plugins().size() % 2 == 0) {
            memcpy(out, in, frames * channels * sizeof(f64));
        }
    }));

    for (auto& plugin : vst2_plugins) {
        TRY(audio_pipeline.pipe(vst2_process_audio(&pipe_arena, &plugin)));
    }

    auto context = Context {
        .pipeline = &audio_pipeline,
        .write_sample = stream_writer.writer,
        .played_frames = &played_frames,
        .frame_count = audio.frame_count,
        .scratch = TRY(arena->alloc_many<f64>(SOUNDIO_MAX_CHANNELS * (usize)audio.sample_rate).or_error(Error::from_string_literal("could not create scratch buffer"))),
        .scratch2 = TRY(arena->alloc_many<f64>(SOUNDIO_MAX_CHANNELS * (usize)audio.sample_rate).or_error(Error::from_string_literal("could not create scratch buffer 2"))),
    };

    SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        return Error::from_errno(ENOMEM);
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->sample_rate = (i32)audio.sample_rate;
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

    dprintln("\n----------------------");
    dprintln("Layout: {}", (u64)audio.sample_layout);
    dprintln("Format: {}", (u64)audio.sample_format);
    dprintln("Sample rate: {}", audio.sample_rate);
    dprintln("Channels: {}", audio.channel_count);
    dprintln("Duration: {}", part_time((u32)audio.duration()));
    dprintln("----------------------\n");

    auto duration = part_time((u32)audio.duration());
    dprint("Time: {} / {}", part_time(0), duration);
    while (!ui_window_should_close(root_window)) {
        ui_application_poll_events(app);
        if (played_frames >= audio.frame_count) {
            break;
        }
        soundio_flush_events(soundio);
        for (auto& plugin : vst2_plugins) {
            (void)plugin.vst->editor_idle();
        }

        auto current_time = part_time(played_frames / audio.sample_rate);
        dprint("\r\033[KTime: {} / {}", current_time, duration);

        usleep(16000);
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

static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_f64(FixedArena* arena_instance, MS::Plugin* plugin);
static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_f32(FixedArena* arena_instance, MS::Plugin* plugin);
static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_audio(FixedArena* arena_instance, MS::Plugin* plugin)
{
    if (plugin->supports_f64()) {
        return vst2_process_f64(arena_instance, plugin);
    }
    return vst2_process_f32(arena_instance, plugin);
}

static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_f64(FixedArena* arena_instance, MS::Plugin* plugin)
{
    return [=](f64* out, f64* in, usize frames, usize channels){
        arena_instance->drain();
        auto* arena = &arena_instance->allocator;

        f64* deinterlaced_out = au_deinterlace_f64(arena, out, frames, channels);
        if (!deinterlaced_out) return; // FIXME: Report error maybe?
        f64* deinterlaced_in = au_deinterlace_f64(arena, in, frames, channels);
        if (!deinterlaced_in) return; // FIXME: Report error maybe?

        f64** outputs = au_shallow_split_channels_f64(arena, plugin->number_of_outputs(), deinterlaced_out, frames, channels);
        if (!outputs) return; // FIXME: Report error maybe?
        f64** inputs = au_shallow_split_channels_f64(arena, plugin->number_of_inputs(), deinterlaced_in, frames, channels);
        if (!inputs) return; // FIXME: Report error maybe?

        plugin->process_f64(outputs, inputs, (i32)frames);

        f64* interlaced_out = au_interlace_f64(arena, deinterlaced_out, frames, channels);
        if (!interlaced_out) return; // FIXME: Report error maybe?
        for (usize i = 0; i < frames * channels; i++) {
            out[i] = interlaced_out[i];
        }
    };
}

static SmallCapture<void(f64*, f64*, usize, usize)> vst2_process_f32(FixedArena* arena_instance, MS::Plugin* plugin)
{
    return [=](f64* out, f64* in, usize frames, usize channels){
        arena_instance->drain();
        auto* arena = &arena_instance->allocator;

        f32* deinterlaced_out = au_deinterlace_f32_from_f64(arena, out, frames, channels);
        if (!deinterlaced_out) return; // FIXME: Report error maybe?

        f32* deinterlaced_in = au_deinterlace_f32_from_f64(arena, in, frames, channels);
        if (!deinterlaced_in) return; // FIXME: Report error maybe?

        f32** outputs = au_shallow_split_channels_f32(arena, plugin->number_of_outputs(), deinterlaced_out, frames, channels);
        if (!outputs) return; // FIXME: Report error maybe?

        f32** inputs = au_shallow_split_channels_f32(arena, plugin->number_of_inputs(), deinterlaced_in, frames, channels);
        if (!inputs) return; // FIXME: Report error maybe?

        plugin->process_f32(outputs, inputs, (i32)frames);

        f64* interlaced_out = au_interlace_f64_from_f32(arena, deinterlaced_out, frames, channels);
        if (!interlaced_out) return; // FIXME: Report error maybe?
        for (usize i = 0; i < frames * channels; i++) {
            out[i] = interlaced_out[i];
        }
    };
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
