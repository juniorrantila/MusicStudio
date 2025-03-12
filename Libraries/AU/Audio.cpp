#include "./Audio.h"

#include <Math/Math.h>
#include <Ty/Allocator.h>
#include <Ty/Defer.h>

#include "./WAV.h"

namespace AU {

static ErrorOr<Audio> transcode(Allocator* arena, u32 sample_rate, WAV wav);
static Optional<f64> sample_frac(View<f64> frames, usize channel_count, f64 frame, usize channel);

ErrorOr<usize> Audio::samples_byte_size(AudioFormat format, Bytes bytes)
{
    switch (format) {
    case AudioFormat::WAV: {
        auto wav = TRY(WAV::decode(bytes));
        return sizeof(f64) * wav.frame_count() * wav.channel_count();
    }
    }
}

ErrorOr<Audio> Audio::decode(Allocator* arena, AudioFormat format, Bytes bytes)
{
    switch (format) {
    case AudioFormat::WAV: {
        auto wav = TRY(WAV::decode(bytes));
        return TRY(transcode(arena, wav.sample_rate(), wav));
    }
    }
}

ErrorOr<Audio> Audio::decode_with_sample_rate(Allocator* arena, u32 sample_rate, AudioFormat format, Bytes bytes)
{
    switch (format) {
    case AudioFormat::WAV: {
        return TRY(transcode(arena, sample_rate, TRY(WAV::decode(bytes))));
    }
    }
}

static ErrorOr<Audio> transcode(Allocator* arena, u32 sample_rate, WAV wav)
{
    if (sample_rate == wav.sample_rate()) {
        auto out_samples = TRY(arena->alloc_many<f64>(wav.frame_count() * wav.channel_count()).or_error(Error::from_errno(ENOMEM)));
        Defer free_out_samples = [&]{
            arena->free_many(out_samples);
        };
        TRY(wav.write_into(out_samples));
        free_out_samples.disarm();
        return Audio(out_samples, AudioSpec{
            .frame_count = wav.frame_count(),
            .channel_count = wav.channel_count(),
            .sample_rate = wav.sample_rate(),
        });
    }
    f64 sample_ratio = (f64)sample_rate / (f64)wav.sample_rate();
    usize frame_count = (usize)(((f64)wav.frame_count()) * sample_ratio);
    auto out_samples = TRY(arena->alloc_many<f64>(sample_ratio * (wav.frame_count() + wav.frame_count() % 16 + 1) * wav.channel_count()).or_error(Error::from_errno(ENOMEM)));
    Defer free_out_samples = [&]{
        arena->free_many(out_samples);
    };
    auto in_samples = TRY(arena->alloc_many<f64>((wav.frame_count() + wav.frame_count() % 16 + 1) * wav.channel_count()).or_error(Error::from_errno(ENOMEM)));
    Defer free_in_samples = [&]{
        arena->free_many(in_samples);
    };

    TRY(wav.write_into(in_samples));

    auto channel_count = wav.channel_count();
    for (usize new_frame = 0; new_frame < frame_count; new_frame++) {
        f64 old_frame = new_frame * (1 / sample_ratio);
        for (usize channel = 0; channel < channel_count; channel++) {
            u32 dest_sample = new_frame * channel_count + channel;
            out_samples[dest_sample] = sample_frac(in_samples, channel_count, old_frame, channel).or_default(0.0);
        }
    }

    free_out_samples.disarm();
    return Audio(out_samples, AudioSpec{
        .frame_count = frame_count,
        .channel_count = wav.channel_count(),
        .sample_rate = sample_rate,
    });
    return {};
}

Optional<f64> Audio::sample_frac(f64 frame, usize channel) const
{
    usize left_frame = (usize)math_floor_f64(frame);
    usize right_frame = (usize)math_ceil_f64(frame);
    auto left_sample = sample(left_frame, channel);
    if (!left_sample.has_value()) {
        return {};
    }
    auto right_sample = sample(right_frame, channel).or_default(left_sample.value());
    return math_lerp_f64(left_sample.value(), right_sample, math_frac_f64(frame));
}

ErrorOr<void> Audio::resample(Allocator* arena, u32 new_sample_rate)
{
    if (m_sample_rate == new_sample_rate) {
        return {};
    }

    f64 sample_ratio = (f64)new_sample_rate / (f64)sample_rate();
    usize new_frame_count = (usize)(((f64)frame_count()) * sample_ratio);
    auto new_samples = TRY(arena->alloc_many<f64>(new_frame_count * channel_count()).or_error(Error::from_errno(ENOMEM)));

    for (usize new_frame = 0; new_frame < new_frame_count; new_frame++) {
        f64 source_frame = ((f64)new_frame) * (1.0 / sample_ratio);
        for (usize channel = 0; channel < channel_count(); channel++) {
            usize dest_sample = new_frame * channel_count() + channel;
            new_samples[dest_sample] = sample_frac(source_frame, channel).or_default(0.0);
        }
    }

    m_samples = new_samples;
    m_sample_rate = new_sample_rate;
    m_frame_count = new_frame_count;
    return {};
}

static Optional<f64> sample_frac(View<f64> frames, usize channel_count, f64 frame, usize channel)
{
    usize left_frame = (usize)frame;
    usize right_frame = (usize)frame + 1;
    auto left_sample = frames.at(left_frame * channel_count + channel).or_default(0.0);
    auto right_sample = frames.at(right_frame * channel_count + channel).or_default(left_sample);
    return math_lerp_f64(left_sample, right_sample, math_frac_f64(frame));
}

}
