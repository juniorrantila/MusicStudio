#pragma once
#include <Ty/Id.h>
#include <AU/Audio.h>
#include <Ty/Vector.h>
#include <Ty/View.h>
#include <SoundIo/SoundIo.h>

struct TrackSlice {
    Id<AU::Audio> audio;
    u32 start_frame;
    u8 relative_end_beat;
};

struct SoundSlice {
    TrackSlice* tracks;
    usize tracks_count;
};

struct Playlist {
    SoundSlice* sound_slice_by_beat_slot;
    usize sound_slice_count;
};

struct AudioCommand {
    Id<AU::Audio> audio;
    bool play;
};

struct FrameContext {
    Playlist playlist;
    Vector<AudioCommand> commands;
};

View<AudioCommand const> beat_command(FrameContext* context, usize beat);
View<AudioCommand const> beat_command(FrameContext* context, usize beat)
{
    context->commands.clear();
    auto playlist = context->playlist;
    usize current_slot = beat & 0xFFFFFFFFFFFFFF00;
    usize relative_beat = beat & 0xFF;
    if (current_slot > playlist.sound_slice_count) {
        return {};
    }
    auto sound_slice = playlist.sound_slice_by_beat_slot[current_slot];
    for (usize track_index = 0; track_index < sound_slice.tracks_count; track_index++) {
        auto track = sound_slice.tracks[track_index];
        auto play = track.relative_end_beat < relative_beat;
        MUST(context->commands.append({
            .audio = track.audio,
            .play = play,
        }));
    }
    return context->commands.view();
}

void audio_scheduler(void)
{
    auto ctx = FrameContext();
    auto audios = Vector<AU::Audio>();
    usize beats = 120;

    SoundIoOutStream* streams[1024];

    for (usize i = 0; i < beats; i++) {
        auto commands = beat_command(&ctx, i);
        for (auto command : commands) {
            auto* stream = streams[command.audio.raw()];
            soundio_outstream_pause(stream, !command.play);
        }
    }

    // playlist.current_frame += frames;
    // playlist.current_beat_time += 
}
