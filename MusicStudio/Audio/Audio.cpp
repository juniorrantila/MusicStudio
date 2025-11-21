#include "./Audio.h"

#include "../State.h"

#include <Basic/Bits.h>
#include <Basic/Verify.h>
#include <Basic/Defer.h>
#include <Basic/Context.h>

#include <LibCore/Actor.h>
#include <LibAudio/AudioDecoder.h>

#include <math.h>

[[maybe_unused]]
static f64 combine(f64 a, f64 b)
{
    return (a + b) * 0.5;
}

[[maybe_unused]]
static f64 combine(f64 a, f64 b, f64 c)
{
    return (a + b + c) / 3.0;
}

static f64 timer(f64 t, f64 interval)
{
    if (interval == 0) return 0;
    return fmod(t, interval) / interval;
}

[[maybe_unused]]
static f64 osc(f64 t, f64 interval)
{
    return (timer(t, interval) * 2.0) - 1.0;
}

[[maybe_unused]]
static f64 operator""_hz(long double v)
{
    return 1.0 / (f64)v;
}

[[maybe_unused]]
static f64 operator""_hz(unsigned long long v)
{
    return 1.0 / (f64)v;
}

static f64 pulses_per_second(PersistedSettings const* settings)
{
    f64 value = settings->pulses_per_quarter_note * settings->quarter_notes_per_second;
    VERIFY(value > 0);
    return value;
}

[[maybe_unused]]
static f64 pulse_time(PersistedSettings const* settings, f64 pulse)
{
    f64 pps = pulses_per_second(settings);
    VERIFY(pps > 0);
    return pulse / pps;
}

static f64 pulses_per_frame(PersistedSettings const* settings)
{
    f64 fps = settings->frames_per_second;
    VERIFY(fps > 0);
    return pulses_per_second(settings) / fps;
}

static f64 frames_per_pulse(PersistedSettings const* settings)
{
    f64 ppf = pulses_per_frame(settings);
    VERIFY(ppf > 0);
    return 1.0 / ppf;
}

[[maybe_unused]]
static f64 sample_at_pulse(PersistedSettings const* settings, StableAudio* ctx, StringSlice path, u32 channel, f64 pulse)
{
    auto* audio_manager = &ctx->audio_manager;
    i64 frame = (i64)(frames_per_pulse(settings) * pulse);
    return audio_manager->sample(
        audio_manager->audio(path),
        frame, channel
    );
}

[[maybe_unused]]
static bool within(f64 a, f64 x, f64 b)
{
    return x >= a && x <= b;
}

static f64 sample_at_pulse(PersistedState const* persisted, StableAudio* stable, TransAudio* trans, f64 pulse, f64)
{
    guard (pulse >= 0.0) else return 0;
    guard (ty_is_initialized(persisted)) else return 0;
    auto* settings = persisted->sections.settings;
    guard (ty_is_initialized(settings)) else return 0;
    guard (ty_is_initialized(stable)) else return 0;
    ty_trans_migrate(trans);

    f64 sample = 0;

#if 1
    bool kick_909 = false;
    f64 kick_909_pulse = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 2.0);
    if (within(0, kick_909_pulse, 1)) kick_909 = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/909/BT0A0D3.WAV"s, 0, kick_909_pulse);

    bool kick_808 = false;
    f64 kick_808_pulse = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 4.0 / 2.0);
    if (within(0, kick_808_pulse, 1)) kick_808 = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/808/BD/BD0010.WAV"s, 0, kick_808_pulse);

    bool snare = false;
    f64 snare_pulse = fmod(pulse - pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 1.0);
    if (within(0, snare_pulse, 1)) snare = true;
    sample += 1.2 * sample_at_pulse(settings, stable, "Samples/808/SD/SD0010.WAV"s, 0, snare_pulse);

    bool hihat = false;
    f64 hihat_pulse = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 8.0);
    if (hihat_pulse < 1) hihat = true;
    sample += 1.2 * sample_at_pulse(settings, stable, "Samples/808/CH/CH.WAV"s, 0, hihat_pulse);

    bool symbal = false;
    f64 symbal_pulse = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 2.0 / 1.0);
    if (within(0, symbal_pulse, 1)) symbal = true;
    sample += sample_at_pulse(settings, stable, "Samples/808/CY/CY5010.WAV"s, 0, symbal_pulse);

    if (kick_909 | kick_808 | snare | hihat | symbal) {
        debugf("%s | %s | %s | %s | %s",
            kick_909  ? "kick 909"  : "        ",
            kick_808  ? "kick 808"  : "        ",
            snare ? "snare" : "     ",
            hihat ? "hihat" : "     ",
            symbal ? "symbal" : "     "
        );
    }
#else
    bool kick = false;
    f64 kick_909_pulse = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 1.2 * 1.0000) * 1;
    if (within(0, kick_909_pulse, 1)) kick = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/909/BT0A0D3.WAV"s, 0, kick_909_pulse);

    f64 kick_909_pulse2 = fmod(pulse + pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 1.5 * 1.50000) * 1;
    if (within(0, kick_909_pulse2, 1)) kick = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/909/BT0A0A7.WAV"s, 0, kick_909_pulse2);

    bool snare = false;
    f64 snare_pulse = fmod(pulse - pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 2.0 * 1.00000) * 1;
    if (within(0, snare_pulse, 1)) snare = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/808/SD/SD0010.WAV"s, 0, snare_pulse * 0.9);

    bool hihat = false;
    f64 hihat_pulse = fmod(pulse - pulses_per_second(settings) / 1.0, pulses_per_second(settings) * 1.0 / 2.0 * 1.00000) * 1;
    if (within(0, hihat_pulse, 1)) hihat = true;
    sample += 1.0 * sample_at_pulse(settings, stable, "Samples/808/CH/CH.WAV"s, 0, hihat_pulse);

    if (kick || snare || hihat) {
        debugf("%4s | %5s | %5s", kick ? "kick" : "", snare ? "snare" : "", hihat ? "hihat" : "");
    }
#endif

    return sample;

    // f64 pulses_per_second = audio->pulses_per_quarter_note * audio->quarter_notes_per_second;
    // f64 t = pulse / pulses_per_second;

    // f64 target = sin(t * 2.0 * M_PI * 440.6) * 0.2;
    // f64 target = sin(t * 2.0 * M_PI * 574.6) * 0.2;
    // f64 target = sin(t * 2.0 * M_PI * 479) * 0.2;

    // return target;
}

C_API void audio_actor_frame(PersistedState const* persisted, StableAudio* stable, TransAudio* trans, f64* const* channels, u32 frame_count, u32 channel_count);
C_API void audio_actor_frame(PersistedState const* persisted, StableAudio* stable, TransAudio* trans, f64* const* channels, u32 frame_count, u32 channel_count)
{
    guard (persisted->version >= sizeof(*persisted)) else {
        return;
    }
    guard (stable->version >= sizeof(*stable)) else {
        return;
    }
    ty_trans_migrate(trans);
    auto* settings = persisted->sections.settings;
    auto* playback = persisted->sections.playback;

    f64 pulses_per_second = settings->pulses_per_quarter_note * settings->quarter_notes_per_second;
    f64 pulses_per_frame = pulses_per_second / settings->frames_per_second;

    f64 current_pulse_offset = playback->current_pulse_offset;
    f64 current_pulse = playback->current_pulse;

    for (u32 frame = 0; frame < frame_count; frame += 1) {
        f64 sample = 0.0;
        if (pulses_per_frame > 1.0) {
            for (i64 pulse = 0; pulse < (i64)pulses_per_frame; pulse += 1) {
                f64 offset = (f64)pulse;
                sample += sample_at_pulse(persisted, stable, trans, current_pulse + offset, fmod(current_pulse_offset + offset, 1));
            }
            sample /= pulses_per_frame;
        } else {
            sample = sample_at_pulse(persisted, stable, trans, current_pulse, current_pulse_offset);
        }
        current_pulse += pulses_per_frame;
        current_pulse_offset = fmod(current_pulse_offset + pulses_per_frame, 1);

        for (u32 channel = 0; channel < channel_count; channel += 1)
            channels[channel][frame] = sample;
    }

    playback->current_pulse = playback->current_pulse + pulses_per_frame * frame_count;
    playback->current_pulse_offset = fmod(playback->current_pulse_offset + pulses_per_frame * frame_count, 1);
}

C_API [[nodiscard]] bool audio_actor_init(Actor* actor, FSVolume* volume, bool use_auto_reload)
{
    if (!use_auto_reload) {
        *actor = actor_init((void(*)(void))audio_actor_frame);
        return true;
    }

    auto path = actor_library_path("music-studio-audio");

    FileID file;
    if (!fs_volume_find(volume, path, &file)) {
        errorf("could not find '%.*s'", (int)path.count, path.items);
        return false;
    }

    if (!actor_init_reloadable(actor, volume, file, "audio_actor_frame")) {
        errorf("could not create actor from library");
        return false;
    }
    return true;
}
