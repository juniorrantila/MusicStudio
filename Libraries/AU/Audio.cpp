#include "./Audio.h"

#include <Math/Math.h>

#include "./WAV.h"

namespace AU {

static ErrorOr<Audio> transcode(WAV wav);

ErrorOr<Audio> Audio::decode(AudioFormat format, Bytes bytes)
{
    switch (format) {
    case AudioFormat::WAV: {
        return TRY(transcode(TRY(WAV::decode(bytes))));
    }
    }
}

ErrorOr<Audio> Audio::decode_with_sample_rate(u32 sample_rate, AudioFormat format, Bytes bytes)
{
    auto audio = TRY(decode(format, bytes));
    TRY(audio.resample(sample_rate));
    return audio;
}

static ErrorOr<Audio> transcode(WAV wav)
{
    auto samples = TRY(Buffer<f64>::create(wav.frame_count() * wav.channel_count()));
    TRY(wav.write_into(&samples));
    return Audio(move(samples), AudioSpec{
        .frame_count = wav.frame_count(),
        .channel_count = wav.channel_count(),
        .sample_rate = wav.sample_rate(),
    });
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

ErrorOr<void> Audio::resample(u32 new_sample_rate)
{
    if (m_sample_rate == new_sample_rate) {
        return {};
    }

    f64 sample_ratio = (f64)new_sample_rate / (f64)sample_rate();
    usize new_frame_count = (usize)(((f64)frame_count()) * sample_ratio);
    auto new_samples = TRY(Buffer<f64>::create(new_frame_count * channel_count()));

    // FIXME: Interpolate if sample_ratio has fraction.
    for (usize new_frame = 0; new_frame < new_frame_count; new_frame++) {
        f64 source_frame = ((f64)new_frame) * (1.0 / sample_ratio);
        for (usize channel = 0; channel < channel_count(); channel++) {
            usize dest_sample = new_frame * channel_count() + channel;
            new_samples[dest_sample] = sample_frac(source_frame, channel).or_default(0.0);
        }
    }

    m_samples = move(new_samples);
    m_sample_rate = new_sample_rate;
    m_frame_count = new_frame_count;
    return {};
}

}
