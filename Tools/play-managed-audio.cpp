#include <Basic/Bits.h>
#include <Basic/DeferredFileLogger.h>
#include <Basic/FileLogger.h>
#include <Basic/MemoryPoker.h>
#include <Basic/PageAllocator.h>
#include <Basic/StringSlice.h>

#include <LibAudio/AudioManager.h>
#include <LibAudio/SoundIo.h>
#include <LibCLI/ArgumentParser.h>
#include <LibCore/FSVolume.h>
#include <LibCore/MappedFile.h>
#include <LibMain/Main.h>

#include <SoundIo/SoundIo.h>

#include <stdio.h>
#include <unistd.h>

struct PartTime {
    u32 hours;
    u8 minutes;
    u8 seconds;
};
static PartTime part_time(u32 milliseconds);

static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
static void underflow_callback(SoundIoOutStream *outstream);

struct Context {
    FileLogger audio_manager_log;
    DeferredFileLogger audio_log;
    AUAudioManager audio_manager;
    StringSlice audio_name;

    Mailbox mailbox;
    MemoryPoker memory_poker;

    void (*write_sample)(void* ptr, f64 sample);
    i32 sample_rate;
    i32 next_print;
    i32 played_frames;
    i32 frame_count;
};

ErrorOr<int> Main::main(int argc, c_string argv[]) {
    auto argument_parser = CLI::ArgumentParser();
    
    c_string wav_path = nullptr;
    TRY(argument_parser.add_positional_argument("wav-path", [&](c_string arg) {
        wav_path = arg;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    static auto log = file_logger_init(stderr);

    Context* context = (Context*)page_alloc(sizeof(*context));
    memory_poker_init(&context->memory_poker);

    if (!mailbox_init(64 * KiB, &context->mailbox).ok)
        return Error::from_string_literal("could not initialize mailbox");
    context->mailbox.attach_memory_poker(&context->memory_poker);

    context->audio_log = deferred_file_logger_init(wav_path, &context->mailbox, stderr);
    context->audio_name = sv_from_c_string(wav_path);
    if (!au_audio_manager_init(&context->audio_manager, &context->memory_poker))
        return Error::from_string_literal("could not initialize audio manager");

    if (!context->memory_poker.start()) {
        log->error("could not start memory poker");
    }

    {
        auto wav_file = TRY(Core::MappedFile::open(wav_path));
        AUAudio wav;
        if (au_audio_decode_wav(wav_file.bytes(), &wav))
            return Error::from_string_literal("could not decode audio");
        context->sample_rate = (i32)wav.sample_rate;
        context->frame_count = (i32)wav.frame_count;
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
    AUSoundIoWriter stream_writer = {};
    if (!au_select_soundio_writer_for_device(device, &stream_writer)) {
        return Error::from_string_literal("could find suitable stream format");
    }
    context->write_sample = stream_writer.writer;

    SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        return Error::from_errno(ENOMEM);
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->sample_rate = context->sample_rate;
    outstream->userdata = context;
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

    for (;;) {
        soundio_flush_events(soundio);
        if (context->played_frames >= context->frame_count)
            break;

        context->mailbox.reader()->wait();
        u16 tag = 0;
        if (!context->mailbox.reader()->peek(&tag).found)
            continue;
        if (tag == Ty2::type_id<decltype(nullptr)>())
            break;
        DeferredLogEvent event;
        if (!context->mailbox.reader()->read(&event).ok)
            continue;
        context->audio_log.handle_event(&event);
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
            ctx->audio_manager_log->error("unrecoverable stream error: %s", soundio_strerror(err));
            return;
        }

        if (!frame_count)
            break;

        SoundIoChannelLayout const* layout = &outstream->layout;
        usize channel_count = layout->channel_count;
        for (int frame = 0; frame < frame_count; frame += 1, ctx->played_frames += 1) {
            if (ctx->played_frames >= ctx->next_print) {
                ctx->next_print = ctx->played_frames + ctx->sample_rate;
                auto current_time = part_time(ctx->played_frames / ctx->sample_rate);
                auto end_time = part_time(ctx->frame_count / ctx->sample_rate);
                ctx->audio_log->info(
                    "%02dh%02dm%02ds / %02dh%02dm%02ds",
                    current_time.hours, current_time.minutes, current_time.seconds,
                    end_time.hours, end_time.minutes, end_time.seconds
                );
            }

            for (usize channel = 0; channel < channel_count; channel += 1) {
                f64 sample = ctx->audio_manager.sample(ctx->audio_manager.audio(ctx->audio_name), ctx->played_frames, channel);
                ctx->write_sample(areas[channel].ptr, sample);
                areas[channel].ptr += areas[channel].step;
            }
        }

        if (auto err = soundio_outstream_end_write(outstream)) {
            if (err == SoundIoErrorUnderflow)
                return;
            ctx->audio_manager_log->error("unrecoverable stream error: %s", soundio_strerror(err));
            return;
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    if (ctx->played_frames >= ctx->frame_count) {
        while (!ctx->mailbox.writer()->post(nullptr).ok);
        soundio_outstream_pause(outstream, true);
    }
}

static void underflow_callback(SoundIoOutStream *outstream) {
    (void)outstream;
    static usize count = 0;
    auto* ctx = (Context*)outstream->userdata;
    ctx->audio_manager_log->warning("underflow %zu", ++count);
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
