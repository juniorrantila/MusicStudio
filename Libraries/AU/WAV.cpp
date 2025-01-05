#include "./WAV.h"
#include "Ty/Limits.h"

#include <Ty/Error.h>
#include <Ty/ErrorOr.h>
#include <Ty/ByteDecoder.h>
#include <Ty/Try.h>
#include <Ty/Buffer.h>

namespace AU {

ErrorOr<WAV> WAV::decode(Bytes bytes)
{
    auto decoder = ByteDecoder(bytes);
    if (!decoder.expect("RIFF")) {
        return Error::from_string_literal_with_errno("file is not a WAV file");
    }
    auto file_size = TRY(decoder.parse_u32le().or_throw([]{
        return Error::from_string_literal("could not decode file size");
    }));
    (void)file_size;
    if (!decoder.expect("WAVE")) {
        return Error::from_string_literal("file is not a WAV file");
    }
    if (!decoder.expect("fmt ")) {
        return Error::from_string_literal("WAV file is missing format chunk");
    }
    auto format_block_size = TRY(decoder.parse_u32le().or_throw([]{
        return Error::from_string_literal("could not decode format block size");
    }));
    auto format_block = TRY(decoder.parse_bytes(format_block_size).or_throw([]{
        return Error::from_string_literal("format block size did not match what was expected");
    }));
    auto format_parser = ByteDecoder(format_block);
    auto format = TRY(format_parser.parse_u16le().map([](u16 format) {
        switch (format) {
        case 1: return WAVAudioFormat::PCM;
        case 3: return WAVAudioFormat::Float;
        }
        return WAVAudioFormat::Unknown;
    }).or_throw([]{
        return Error::from_string_literal("could not parse audio format");
    }));

    auto channel_count = TRY(format_parser.parse_u16le().or_throw([]{
        return Error::from_string_literal("could not decode channel count");
    }));

    auto sample_rate = TRY(format_parser.parse_u32le().or_throw([]{
        return Error::from_string_literal("could not decode sample rate");
    }));

    auto bytes_per_second = TRY(format_parser.parse_u32le().or_throw([]{
        return Error::from_string_literal("could not decode bytes per second");
    }));

    auto bytes_per_block = TRY(format_parser.parse_u16le().or_throw([]{
        return Error::from_string_literal("could not decode bytes per block");
    }));

    auto bits_per_sample = TRY(format_parser.parse_u16le().or_throw([]{
        return Error::from_string_literal("could not decode bits per sample");
    }));

    auto samples = Bytes();
    while (decoder.peeks(4).has_value()) {
        auto section_name = decoder.parse_string(4);
        u32 data_size = TRY(decoder.parse_u32le().or_throw([]{
            return Error::from_string_literal("could not decode section size");
        }));
        if (section_name != "data") {
            decoder.skip(data_size);
            continue;
        }

        samples = TRY(decoder.parse_bytes(data_size).or_throw([]{
            return Error::from_string_literal("data size did not match what was expected");
        }));
    }

    return WAV(samples, {
        .format = format,
        .channel_count = channel_count,
        .sample_rate = sample_rate,
        .bytes_per_second = bytes_per_second,
        .bytes_per_block = bytes_per_block,
        .bits_per_sample = bits_per_sample,
    });
}

static void write_i8(WAV&, usize, View<f64>);
static void write_i16(WAV&, usize, View<f64>);
static void write_i32(WAV&, usize, View<f64>);
static void write_i64(WAV&, usize, View<f64>);
static void write_f32(WAV&, usize, View<f64>);
static void write_f64(WAV&, usize, View<f64>);

ErrorOr<usize> WAV::write_into(View<f64> buffer)
{
    usize samples_to_write = buffer.size() < sample_count() ? buffer.size() : sample_count();
    switch (format()) {
    case AU::WAVAudioFormat::Unknown:
        return Error::from_string_literal("unknown WAV format");
    case WAVAudioFormat::PCM:
        if (bits_per_sample() == 8) {
            write_i8(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        if (bits_per_sample() == 16) {
            write_i16(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        if (bits_per_sample() == 32) {
            write_i32(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        if (bits_per_sample() == 64) {
            write_i64(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        return Error::unimplemented();
    case WAVAudioFormat::Float:
        if (bits_per_sample() == 32) {
            write_f32(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        if (bits_per_sample() == 64) {
            write_f64(*this, samples_to_write, buffer);
            return samples_to_write;
        }
        return Error::unimplemented();
    }
    return {};
}

static void write_i8(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        f64 sample = *(i8*)&samples[i * wav.bytes_per_sample()];
        buffer[i] = sample / (f64)Limits<i8>::max();
    }
}

static void write_i16(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        f64 sample = *(i16*)&samples[i * wav.bytes_per_sample()];
        buffer[i] = sample / (f64)Limits<i16>::max();
    }
}

static void write_i32(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        f64 sample = *(i32*)&samples[i * wav.bytes_per_sample()];
        buffer[i] = sample / (f64)Limits<i32>::max();
    }
}

static void write_i64(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        f64 sample = *(i64*)&samples[i * wav.bytes_per_sample()];
        buffer[i] = sample / (f64)Limits<i64>::max();
    }
}

static void write_f32(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        buffer[i] = *(f32*)&samples[i * wav.bytes_per_sample()];
    }
}

static void write_f64(WAV& wav, usize samples_to_write, View<f64> buffer)
{
    auto samples = wav.samples();
    for (usize i = 0; i < samples_to_write; i++) {
        buffer[i] = *(f64*)&samples[i * wav.bytes_per_sample()];
    }
}

}
