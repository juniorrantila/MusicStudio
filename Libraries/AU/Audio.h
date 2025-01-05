#pragma once
#include "./Forward.h"

#include <Ty/Optional.h>
#include <Ty/Forward.h>
#include <Ty/View.h>

namespace AU {

struct AudioSpec {
    usize frame_count;
    usize channel_count;
    usize sample_rate;
};

enum class AudioFormat {
    WAV,
};

struct Audio {
    constexpr Audio() = default;

    constexpr Audio(View<f64> samples, AudioSpec spec)
        : m_samples(move(samples))
        , m_frame_count(spec.frame_count)
        , m_channel_count(spec.channel_count)
        , m_sample_rate(spec.sample_rate)
    {
    }

    static ErrorOr<usize> samples_byte_size(AudioFormat, Bytes);

    static ErrorOr<Audio> decode(ArenaAllocator* arena, AudioFormat, Bytes);
    static ErrorOr<Audio> decode_with_sample_rate(ArenaAllocator* arena, u32 sample_rate, AudioFormat, Bytes);

    Optional<f64> sample(usize frame, usize channel) const
    {
        return m_samples.at(frame * channel_count() + channel);
    }

    Optional<f64> sample_frac(f64 frame, usize channel) const;

    usize frame_count() const { return m_frame_count; }
    u32 channel_count() const { return m_channel_count; }
    u32 sample_rate() const { return m_sample_rate; }
    f64 duration() const { return (f64)frame_count() / (f64)sample_rate(); }
    View<f64 const> samples() const { return m_samples.as_const(); }
    ErrorOr<void> resample(ArenaAllocator* arena, u32 sample_rate);

private:
    View<f64> m_samples {};
    usize m_frame_count { 0 };
    usize m_channel_count { 0 };
    usize m_sample_rate { 0 };
};

}
