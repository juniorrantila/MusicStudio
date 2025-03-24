#include "./Context.h"

#include <AU/SoundIo.h>
#include <CLI/ArgumentParser.h>
#include <Core/Print.h>
#include <Core/Time.h>
#include <FS/Bundle.h>
#include <Fonts/Fonts.h>
#include <GL/GL.h>
#include <MS/PluginManager.h>
#include <Main/Main.h>
#include <Math/Math.h>
#include <SoundIo/SoundIo.h>
#include <SoundIo/os.h>
#include <Ty2/Arena.h>
#include <Ty2/PageAllocator.h>
#include <UI/Application.h>
#include <UI/KeyCode.h>
#include <UI/Window.h>

#include <unistd.h>

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) noexcept [[clang::nonblocking]];

ErrorOr<int> Main::main(int argc, c_string* argv)
{
    auto bundle = FS::Bundle().add_pack(Fonts());
    dprintln("resources:");
    for (auto resource : bundle.resources()) {
        dprintln("  {}", resource.resolved_path());
    }

    auto argument_parser = CLI::ArgumentParser();

    auto root_directory = "."sv;
    TRY(argument_parser.add_option("--root", "-r", "directory", "root directory", [&](c_string arg) {
        root_directory = StringView::from_c_string(arg);
    }));

    auto plugin_paths = Vector<c_string>();
    TRY(argument_parser.add_option("--plugin", "-p", "plugin", "open plugin", [&](c_string arg) {
        MUST(plugin_paths.append(arg));
    }));

    auto hot_reload = ""sv;
    TRY(argument_parser.add_option("--hot-reload", "-h", "shared-library", "hot reload", [&](c_string arg) {
        hot_reload = StringView::from_c_string(arg);
    }));

    u32 app_hints = 0;
    TRY(argument_parser.add_flag("--native-like", "-nl", "run native-like mode", [&]() {
        app_hints |= UIApplicationHint_NativeLike;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto* app = ui_application_create(app_hints);
    defer [&] {
        ui_application_destroy(app);
    };

    auto* window = ui_window_create(app, {
        .parent = nullptr,
        .title = "MusicStudio",
        .x = 0,
        .y = 0,
        .width = 900,
        .height = 600,
    });
    defer [&] {
        ui_window_destroy(window);
    };

    auto arena_allocator = arena_create(page_allocator());
    auto* arena = &arena_allocator.allocator;
    auto project = MS::Project();
    auto manager = TRY(MS::PluginManager::create(arena)).init(&project);
    auto plugins = TRY(arena->alloc_many<Id<MS::Plugin>>(1024).or_error(Error::from_errno(ENOMEM)));

    usize plugin_count = 0;
    for (usize i = 0; i < plugin_paths.size(); i++) {
        plugin_count += 1;
        plugins[i] = TRY(manager.instantiate(plugin_paths[i]));
    }
    (void)plugin_count;
    for (usize i = 0; i < plugin_count; i++) {
        manager.plugins[i].init();
    }

    Context context = TRY(context_create(bundle));
    defer [&] {
        context_destroy(&context);
    };

    ui_window_set_resize_callback(window, &context, [](UIWindow* window, void* user) {
        ui_window_gl_make_current_context(window);
        context_window_did_resize((Context*)user, window);
        ui_window_gl_flush(window);
    });

    ui_window_set_scroll_callback(window, &context, [](UIWindow* window, void* user) {
        context_window_did_scroll((Context*)user, window);
    });

    SoundIo* soundio = soundio_create();
    if (!soundio) {
        return Error::from_string_literal("could not create soundio");
    }
    defer [&] {
        soundio_destroy(soundio);
    };

    if (int err = soundio_connect(soundio)) {
        return Error::from_string_literal(soundio_strerror(err));
    }
    soundio_flush_events(soundio);
    int output_device_id = soundio_default_output_device_index(soundio);
    if (output_device_id < 0) {
        return Error::from_string_literal("could not find default output device");
    }
    SoundIoDevice* output_device = soundio_get_output_device(soundio, output_device_id);
    defer [&] {
        soundio_device_unref(output_device);
    };
    SoundIoOutStream* outstream = soundio_outstream_create(output_device);
    if (!outstream) {
        return Error::from_string_literal("could not create out stream");
    }
    defer [&] {
        soundio_outstream_destroy(outstream);
    };
    outstream->write_callback = write_callback;
    outstream->underflow_callback = [](SoundIoOutStream* outstream) {
        auto* context = (Context*)outstream->userdata;
        context->rt.underflow_count++;
    };
    outstream->error_callback = [](SoundIoOutStream*, int error) {
        dprintln("SoundIo error: {}", StringView::from_c_string(soundio_strerror(error)));
    };
    outstream->name = "MusicStudio";
    outstream->software_latency = 0;

    outstream->sample_rate = project.sample_rate;

    soundio->on_backend_disconnect = [](SoundIo*, int reason) {
        dprintln("SoundIo backend disconnect {}", StringView::from_c_string(soundio_strerror(reason)));
    };

    soundio->on_devices_change = [](SoundIo*) {
        dprintln("SoundIo device change");
    };

    auto device_format = TRY(AU::select_writer_for_device(output_device).or_error(Error::from_string_literal("no suitable device format available")));
    outstream->format = device_format.format;
    outstream->userdata = &context;
    context.rt.write = device_format.writer;

    if (int err = soundio_outstream_open(outstream)) {
        return Error::from_string_literal(soundio_strerror(err));
    }

    if (outstream->layout_error) {
        return Error::from_string_literal("unable to set channel layout");
    }

    if (int err = soundio_outstream_start(outstream)) {
        return Error::from_string_literal(soundio_strerror(err));
    }

    {
        ui_window_gl_make_current_context(window);
        auto size = ui_window_size(window);
        auto pixel_ratio = ui_window_pixel_ratio(window);
        layout_set_size(context.layout, size, pixel_ratio);
        glViewport(0, 0, (i32)(size.x * pixel_ratio), (i32)(size.y * pixel_ratio));
    }

    bool d_pressed_last_frame = false;
    bool space_pressed_last_frame = false;
    f64 start = soundio_os_get_time();
    u32 target_fps = 60;
    f64 target_frame_time = 1.0 / (f64)target_fps;
    for (;!ui_window_should_close(window); context.frame++) {
        ui_application_poll_events(app);
        soundio_flush_events(soundio);
        ui_window_gl_make_current_context(window);

        u8 const* keymap = ui_window_keymap(window);
        context_set_notes_from_keymap(&context, Midi::Note::C4, keymap);
        if (keymap[UIKeyCode_7]) {
            if (!d_pressed_last_frame) {
                //Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
                __builtin_memset(context.tracker, 0, sizeof(context.tracker));
            }
        }
        d_pressed_last_frame = keymap[UIKeyCode_D];

        if (keymap[UIKeyCode_SPACE]) {
            if (!space_pressed_last_frame) {
                context.is_playing = !context.is_playing;
                if (context.is_playing) {
                    context.rt.seconds_offset = 0.0;
                    context.current_subdivision = 0;
                }
            }
        }
        space_pressed_last_frame = keymap[UIKeyCode_SPACE];

        context_update(&context, window);

        ui_window_gl_flush(window);

        if (context.frame % target_fps == 0) {
            dprintln("Layout time: {}us", (u32)(context.avg_layout * 1e6));
            dprintln("Render time: {}us", (u32)(context.avg_render * 1e6));
            dprintln("Update time: {}ms", (u32)(context.avg_update * 1e3));
            dprintln("FPS: {}", (u32)(1.0 / context.delta_time));
            dprintln("Audio latency: {}ms", context.rt.latency * 1e3);
            dprintln("Audio process: {}us", context.rt.process_time * 1e6);
        }

        auto t = soundio_os_get_time();
        context.delta_time = t - start;
        start = t;
        if (context.delta_time < target_frame_time) {
            f64 t = (target_frame_time - context.delta_time) * 1e6;
            usleep(t);
        }
    }

    return {};
}


f64 note(f64 x);
f64 note(f64 x)
{
    return math_abs_f64(math_mod_f64(x, 1.0)) * (0.25 + math_mod_f64(x * 0.001, 1) * 0.5) * 1.5;
}

static f64 gen_sample(Context* ctx, f64 time)
{
    u8 const _Atomic* piano_notes = ctx->notes;
    f64 result = 0;

    for (u8 i = 0; i < (u8)Midi::Note::__Size; i++) {
        if (!piano_notes[i]) continue;
        f64 pitch = Midi::note_frequency((Midi::Note)i);
        result += (0.1 * note(time * pitch) * 1.0);
    }

    if (ctx->is_playing) {
        f64 current_beat = math_mod_f64(time, ctx->beats_per_minute);
        u32 current_subdivision = (u32)(math_mod_f64(current_beat, 1.0) * ctx->subdivisions);

        ctx->current_subdivision = ((u32)current_beat) * ctx->subdivisions + current_subdivision;

        auto const& notes = ctx->tracker[(u32)current_beat][current_subdivision];
        for (u8 i = 0; i < (u8)Midi::Note::__Size; i++) {
            if (!notes[i]) continue;
            f64 pitch = Midi::note_frequency((Midi::Note)i);
            result += (0.1 * note(time * pitch) * 1.0);
        }
    }

    return result;
}

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) noexcept [[clang::nonblocking]]
{
    (void)frame_count_min;
    auto* ctx = (Context*)outstream->userdata;
    f64 float_sample_rate = outstream->sample_rate;
    f64 seconds_per_frame = 1.0 / float_sample_rate;

    f64 start = soundio_os_get_time();

    int frames_left = frame_count_max;
    for (;;) {
        int frame_count = frames_left;
        SoundIoChannelArea* areas = nullptr;
        if (int err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
            dprintln("unrecoverable stream error: {}", StringView::from_c_string(soundio_strerror(err)));
            return;
        }

        if (!frame_count)
            break;

        const SoundIoChannelLayout *layout = &outstream->layout;

        for (int frame = 0; frame < frame_count; frame += 1) {
            f64 sample = gen_sample(ctx, ctx->rt.seconds_offset + frame * seconds_per_frame);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                ctx->rt.write(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }
        ctx->rt.seconds_offset = ctx->rt.seconds_offset + seconds_per_frame * frame_count;
        // ctx->rt.seconds_offset = fmod(ctx->rt.seconds_offset + seconds_per_frame * frame_count, 1.0);

        if (int err = soundio_outstream_end_write(outstream)) {
            if (err == SoundIoErrorUnderflow)
                return;
            dprintln("unrecoverable stream error: {}", StringView::from_c_string(soundio_strerror(err)));
            return;
        }

        f64 latency = 0.0;
        if (soundio_outstream_get_latency(outstream, &latency) == 0) {
            ctx->rt.latency = latency;
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    ctx->rt.process_time = soundio_os_get_time() - start;
}
