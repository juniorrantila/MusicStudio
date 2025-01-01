#pragma once
#include "./Forward.h"

#include <Ty/Optional.h>
#include <Ty/Buffer.h>

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

    constexpr Audio(Buffer<f64>&& samples, AudioSpec spec) 
        : m_samples(move(samples))
        , m_frame_count(spec.frame_count)
        , m_channel_count(spec.channel_count)
        , m_sample_rate(spec.sample_rate)
    {
    }

    static ErrorOr<Audio> decode(AudioFormat, Bytes);

    Optional<f64> sample(usize frame, usize channel) const
    {
        return m_samples.at(frame * channel_count() + channel);
    }

    usize frame_count() const { return m_frame_count; }
    usize channel_count() const { return m_channel_count; }
    usize sample_rate() const { return m_sample_rate; }
    f64 duration() const { return (f64)frame_count() / (f64)sample_rate(); }

private:
    Buffer<f64> m_samples {};
    usize m_frame_count { 0 };
    usize m_channel_count { 0 };
    usize m_sample_rate { 0 };
};

}
