#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/Bytes.h>

namespace AU {

enum class WAVAudioFormat : i32 {
    Unknown = -1,
    PCM = 1,
    Float = 3,
};

struct WAVFormat {
    WAVAudioFormat format { WAVAudioFormat::Unknown };
    u16 channel_count { 0 };
    u32 sample_rate { 0 };
    u32 bytes_per_second { 0 };
    u32 bytes_per_block { 0 };
    u16 bits_per_sample { 0 };
};

struct WAV {
    constexpr WAV(Bytes samples, WAVFormat format)
        : m_format(format)
        , m_samples(samples)
    {
    }

    static ErrorOr<WAV> decode(Bytes);

    WAVAudioFormat format() const { return m_format.format; }
    usize channel_count() const { return m_format.channel_count; }
    usize sample_rate() const { return m_format.sample_rate; }
    usize bytes_per_second() const { return m_format.bytes_per_second; }
    usize bytes_per_block() const { return m_format.bytes_per_block; }
    usize bits_per_sample() const { return m_format.bits_per_sample; }
    usize bytes_per_sample() const { return bits_per_sample() / 8; }
    Bytes samples() const { return m_samples; }

    usize frame_count() const
    {
        return samples().size() / bytes_per_block();
    }

    usize sample_count() const
    {
        return frame_count() * channel_count();
    }

    ErrorOr<usize> write_into(Buffer<f64>*);

private:
    WAVFormat m_format {};
    Bytes m_samples {};

};

}
