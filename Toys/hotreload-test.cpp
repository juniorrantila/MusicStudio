#include <Main/Main.h>
#include <Library/Library.h>
#include <Library/HotReload.h>
#include <Ty2/Arena.h>
#include <Ty2/PageAllocator.h>
#include <FS/FSVolume.h>
#include <Ty2/FileLogger.h>
#include <FS/OS.h>
#include <SoundIo/SoundIo.h>

#include <math.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>

struct HotLib {
    void* state;
    f64 (*sample_at_time)(void*, f64 time);
};

struct AudioContext {
    HotLib hotlib;
    f64 seconds_offset;
};

struct Context {
    Allocator* arena;
    Logger* log;
    FSVolume* volume;
};

static SoundIoOutStream* create_default_outstream(SoundIo*);
static Library* hotlib_create(Context const*);
static HotLib hotlib_mount(Library const*);

ErrorOr<int> Main::main(int, c_string[])
{
    Arena arena_instance = arena_create(page_allocator());
    Allocator* arena = &arena_instance.allocator;
    FileLogger file_logger = make_file_logger(arena, stderr);
    Logger* log = &file_logger.logger;
    FSVolume* volume = fs_volume_create(arena);
    if (!volume) log->fatal("could not create volume");

    if (fs_is_system_integrity_protection_enabled()) {
        log->warning("This program **will** crash when reloading libraries unless you turn off 'System Integrity Protection'");
    }

    auto ctx = Context{
        .arena = arena,
        .log = log,
        .volume = volume,
    };
    auto* lib = hotlib_create(&ctx);
    if (!lib) log->fatal("could not create hotlib");

    auto audio_context = AudioContext{
        .hotlib = hotlib_mount(lib),
        .seconds_offset = 0.0,
    };

    SoundIo* soundio = soundio_create();
    soundio_connect(soundio);
    soundio_flush_events(soundio);
    auto* outstream = create_default_outstream(soundio);
    outstream->userdata = &audio_context;
    soundio_outstream_open(outstream);
    soundio_outstream_start(outstream);

    static jmp_buf panic_handler;
    (void)signal(SIGSEGV, [](int){ longjmp(panic_handler, 1); });
    if (setjmp(panic_handler) == 1) {
        log->info("recovered from panic");
    }
    while (1) {
        soundio_flush_events(soundio);

        struct timespec timeout = {
            .tv_sec = 0,
            .tv_nsec = 1000000,
        };
        fs_volume_poll_events(volume, &timeout);
        library_update(lib);
        if (library_needs_reload(lib)) {
            soundio_outstream_pause(outstream, true);
            bool success = library_reload(lib);
            audio_context.hotlib = hotlib_mount(lib);
            soundio_outstream_pause(outstream, false);
            if (success) log->info("reloaded audio library");
            else log->warning("could not reload audio library");
        }
    }

    return 0;
}

static Library* hotlib_create(Context const* ctx)
{
    auto path = HOTRELOAD_LIB_DIR "/hotreload-test-lib.hotlib"s;

    FSFile file;
    if (!fs_system_open(ctx->arena, path, &file)) {
        ctx->log->error("could not open '%.*s'", (int)path.count, path.items);
        return nullptr;
    }

    FileID file_id;
    if (!fs_volume_mount(ctx->volume, file, &file_id)) {
        ctx->log->error("could not mount '%.*s'", (int)path.count, path.items);
        return nullptr;
    }

    e_library error;
    Library* lib = library_hotreloadable(ctx->arena, ctx->volume, file_id, &error);
    if (!lib) {
        ctx->log->error("could not load '%.*s': %s", (int)path.count, path.items, library_strerror(error));
        return nullptr;
    }

    return lib;
}

HotLib hotlib_mount(Library const* lib)
{
    return (HotLib){
        .state = library_hotreload_state(lib),
        .sample_at_time = (f64(*)(void*, f64))library_get_symbol(lib, "sample_at_time"),
    };
}

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static SoundIoOutStream* create_default_outstream(SoundIo* soundio)
{
    int selected_device_index = soundio_default_output_device_index(soundio);
    auto* device = soundio_get_output_device(soundio, selected_device_index);
    auto* outstream = soundio_outstream_create(device);
    outstream->write_callback = write_callback;
    outstream->format = SoundIoFormatFloat64NE;
    return outstream;
}

static void write_callback(SoundIoOutStream* outstream, int, int frame_count_max) {
    f64 float_sample_rate = outstream->sample_rate;
    f64 seconds_per_frame = 1.0 / float_sample_rate;
    SoundIoChannelArea* areas = nullptr;
    auto* ctx = (AudioContext*)outstream->userdata;
    auto* hotlib = &ctx->hotlib;

    f64 seconds_offset = ctx->seconds_offset;
    int frames_left = frame_count_max;
    for (;;) {
        int frame_count = frames_left;
        if (auto err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
            (void)fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            _Exit(1);
        }

        if (!frame_count)
            break;

        auto* layout = &outstream->layout;
        for (int frame = 0; frame < frame_count; frame += 1) {
            f64 sample = 0.0;
            if (hotlib->sample_at_time) {
                sample = hotlib->sample_at_time(hotlib->state, seconds_offset + frame * seconds_per_frame);
            }
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                *(f64*)areas[channel].ptr = sample;
                areas[channel].ptr += areas[channel].step;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);

        if (auto err = soundio_outstream_end_write(outstream)) {
            if (err == SoundIoErrorUnderflow)
                return;
            (void)fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            _Exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    ctx->seconds_offset = seconds_offset;
}
