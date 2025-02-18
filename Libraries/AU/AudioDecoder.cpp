#include "./AudioDecoder.h"

#include <Schedule/Schedule.h>
#include <Ty/ByteDecoder.h>
#include <Ty/Verify.h>

#include <Ty/Limits.h>
#include <Ty/Defer.h>

#include <string.h>

u64 au_audio_sample_count(AUAudio const* audio)
{
    return audio->frame_count * audio->channel_count;
}

static u64 bytes_per_sample(AUSampleFormat format)
{
    return format >> 1;
}

u64 au_audio_bytes_per_sample(AUAudio const* audio)
{
    return bytes_per_sample(audio->sample_format);
}

u64 au_audio_byte_size(AUAudio const* audio)
{
    return au_audio_sample_count(audio) * au_audio_bytes_per_sample(audio);
}

void au_audio_destroy(AUAudio* audio)
{
    auto size = au_audio_byte_size(audio);
    audio->gpa->raw_free(audio->samples.i8, size);
}

static usize sample_index(AUAudio const* audio, usize channel, usize frame)
{
    switch (audio->sample_layout) {
    case AUSampleLayout_Interlaced: return frame * audio->channel_count + channel;
    case AUSampleLayout_Linear: return channel * audio->frame_count + frame;
    }
}

i8 au_audio_sample_i8(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return audio->samples.i8[index];
    case AUSampleFormat_I16:
        return (i8)(audio->samples.i16[index] / 2);
    case AUSampleFormat_I32:
        return (i8)(audio->samples.i32[index] / 4);
    case AUSampleFormat_I64:
        return (i8)(audio->samples.i64[index] / 8);
    case AUSampleFormat_F16:
        return (i8)(audio->samples.f16[index] * (f16)Limits<i8>::max());
    case AUSampleFormat_F32:
        return (i8)(audio->samples.f32[index] * (f32)Limits<i8>::max());
    case AUSampleFormat_F64:
        return (i8)(audio->samples.f64[index] * (f32)Limits<i8>::max());
    }
}

i16 au_audio_sample_i16(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (i16)(((i16)audio->samples.i8[index]) * 2);
    case AUSampleFormat_I16:
        return audio->samples.i16[index];
    case AUSampleFormat_I32:
        return (i16)(audio->samples.i32[index] / 2);
    case AUSampleFormat_I64:
        return (i16)(audio->samples.i64[index] / 4);
    case AUSampleFormat_F16:
        return (i16)(audio->samples.f16[index] * (f16)Limits<i16>::max());
    case AUSampleFormat_F32:
        return (i16)(audio->samples.f32[index] * (f32)Limits<i16>::max());
    case AUSampleFormat_F64:
        return (i16)(audio->samples.f64[index] * (f32)Limits<i16>::max());
    }
}

i32 au_audio_sample_i32(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (i32)(((i32)audio->samples.i8[index]) * 4);
    case AUSampleFormat_I16:
        return (i32)(((i32)audio->samples.i16[index]) * 2);
    case AUSampleFormat_I32:
        return audio->samples.i32[index];
    case AUSampleFormat_I64:
        return (i32)(audio->samples.i64[index] / 2);
    case AUSampleFormat_F16:
        return (i32)(audio->samples.f16[index] * (f16)Limits<i32>::max());
    case AUSampleFormat_F32:
        return (i32)(audio->samples.f32[index] * (f32)Limits<i32>::max());
    case AUSampleFormat_F64:
        return (i32)(audio->samples.f64[index] * (f32)Limits<i32>::max());
    }
}

i64 au_audio_sample_i64(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (i64)(((i64)audio->samples.i8[index]) * 8);
    case AUSampleFormat_I16:
        return (i64)(((i64)audio->samples.i16[index]) * 4);
    case AUSampleFormat_I32:
        return (i64)(((i64)audio->samples.i32[index]) * 2);
    case AUSampleFormat_I64:
        return audio->samples.i64[index];
    case AUSampleFormat_F16:
        return (i64)(audio->samples.f16[index] * (f16)Limits<i64>::max());
    case AUSampleFormat_F32:
        return (i64)(audio->samples.f32[index] * (f32)Limits<i64>::max());
    case AUSampleFormat_F64:
        return (i64)(audio->samples.f64[index] * (f32)Limits<i64>::max());
    }
}

f16 au_audio_sample_f16(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0.0f;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (f16)(((f16)audio->samples.i8[index]) / (f16)Limits<i8>::max());
    case AUSampleFormat_I16:
        return (f16)(((f32)audio->samples.i16[index]) / (f32)Limits<i16>::max());
    case AUSampleFormat_I32:
        return (f16)(((f64)audio->samples.i32[index]) / (f64)Limits<i32>::max());
    case AUSampleFormat_I64:
        return (f16)(((f64)audio->samples.i64[index]) / (f64)Limits<i64>::max());
    case AUSampleFormat_F16:
        return audio->samples.f16[index];
    case AUSampleFormat_F32:
        return audio->samples.f32[index];
    case AUSampleFormat_F64:
        return audio->samples.f64[index];
    }
}

f32 au_audio_sample_f32(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0.0f;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (f32)(((f32)audio->samples.i8[index]) / (f32)Limits<i8>::max());
    case AUSampleFormat_I16:
        return (f32)(((f32)audio->samples.i16[index]) / (f32)Limits<i16>::max());
    case AUSampleFormat_I32:
        return (f32)(((f64)audio->samples.i32[index]) / (f64)Limits<i32>::max());
    case AUSampleFormat_I64:
        return (f32)(((f64)audio->samples.i64[index]) / (f64)Limits<i64>::max());
    case AUSampleFormat_F16:
        return (f32)audio->samples.f16[index];
    case AUSampleFormat_F32:
        return audio->samples.f32[index];
    case AUSampleFormat_F64:
        return (f32)audio->samples.f64[index];
    }
}

f64 au_audio_sample_f64(AUAudio const* audio, usize channel, usize frame)
{
    usize index = sample_index(audio, channel, frame);
    if (index >= au_audio_sample_count(audio)) {
        return 0.0f;
    }
    switch (audio->sample_format) {
    case AUSampleFormat_I8:
        return (f64)(((f64)audio->samples.i8[index]) / (f64)Limits<i8>::max());
    case AUSampleFormat_I16:
        return (f64)(((f64)audio->samples.i16[index]) / (f64)Limits<i16>::max());
    case AUSampleFormat_I32:
        return (f64)(((f64)audio->samples.i32[index]) / (f64)Limits<i32>::max());
    case AUSampleFormat_I64:
        return (f64)(((f64)audio->samples.i64[index]) / (f64)Limits<i64>::max());
    case AUSampleFormat_F16:
        return (f64)audio->samples.f16[index];
    case AUSampleFormat_F32:
        return audio->samples.f32[index];
    case AUSampleFormat_F64:
        return audio->samples.f64[index];
    }
}

f64 au_audio_duration(AUAudio const* audio)
{
    return ((f64)audio->frame_count) / (f64)audio->sample_rate;
}

e_au_transcode au_transcode(Schedule* schedule, Allocator* gpa, AUAudio input, AUAudioSpec to_spec, AUAudio* out)
{
    if (to_spec.sample_rate == 0) {
        to_spec.sample_rate = input.sample_rate;
    }

    // FIXME
    VERIFY(to_spec.sample_rate == input.sample_rate);

    AUAudio output = {
        .gpa = gpa,
        .sample_layout = to_spec.sample_layout,
        .sample_format = to_spec.sample_format,
        .sample_rate = to_spec.sample_rate,
        .channel_count = input.channel_count,
        .frame_count = input.frame_count,
        .samples = {
            .i8 = nullptr,
        }
    };
    auto byte_size = au_audio_byte_size(&output);
    output.samples.i8 = (i8*)gpa->raw_alloc(byte_size, 16).or_default(nullptr);
    if (!output.samples.i8) {
        return e_au_transcode_out_of_memory;
    }

    if (
        input.sample_format == to_spec.sample_format &&
        input.sample_layout == to_spec.sample_layout
    ) {
        memcpy(output.samples.i8, input.samples.i8, byte_size);
        return *out = output, e_au_transcode_none;
    }

    auto sched = ScheduleRef{schedule};
    switch (output.sample_format) {
    case AUSampleFormat_I8:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.i8[dest_index] = au_audio_sample_i8(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_I16:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.i16[dest_index] = au_audio_sample_i16(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_I32:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.i32[dest_index] = au_audio_sample_i32(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_I64:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.i64[dest_index] = au_audio_sample_i64(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_F16:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.f16[dest_index] = au_audio_sample_f16(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_F32:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.f32[dest_index] = au_audio_sample_f32(&input, channel, frame);
            }
        });
        break;
    case AUSampleFormat_F64:
        sched.parallel_for(input.frame_count, [=](usize frame){
            for (usize channel = 0; channel < input.channel_count; channel++) {
                usize dest_index = sample_index(&output, channel, frame);
                output.samples.f64[dest_index] = au_audio_sample_f64(&input, channel, frame);
            }
        });
        break;
    }

    *out = output;
    return e_au_transcode_none;
}

enum AUWAVFormat {
    AUWAVFormat_Unknown = 0,
    AUWAVFormat_PCM = 1,
    AUWAVFormat_Float = 2,
};

struct AUWAV {
    AUWAVFormat format;
    u16 channel_count;
    u32 sample_rate;
    u32 bytes_per_second;
    u32 bytes_per_block;
    u16 bits_per_sample;
    u32 frame_count;
    union {
        i8*  i8;
        i16* i16;
        i32* i32;
        i64* i64;
        f32* f32;
        f64* f64;
    } samples;
};
static e_au_decode decode_wav(Bytes bytes, AUWAV*);

static e_au_decode match_wav_format(AUWAVFormat format, u16 bits_per_sample, AUSampleFormat* out)
{
    constexpr auto match = [](AUWAVFormat format, u16 bits_per_sample) constexpr {
        return ((bits_per_sample / 8) << 2) | format;
    };
    switch (match(format, bits_per_sample)) {
    case match(AUWAVFormat_PCM,   8):
        return *out = AUSampleFormat_I8, e_au_decode_none;
    case match(AUWAVFormat_PCM,   16):
        return *out = AUSampleFormat_I16, e_au_decode_none;
    case match(AUWAVFormat_PCM,   32):
        return *out = AUSampleFormat_I32, e_au_decode_none;
    case match(AUWAVFormat_PCM,   64):
        return *out = AUSampleFormat_I64, e_au_decode_none;
    case match(AUWAVFormat_Float, 32):
        return *out = AUSampleFormat_F32, e_au_decode_none;
    case match(AUWAVFormat_Float, 64):
        return *out = AUSampleFormat_F64, e_au_decode_none;
    }
    return e_au_decode_unsupported_invariant;
}

static e_au_decode borrow_wav(AUWAV wav, AUAudio* out)
{
    AUSampleFormat sample_format;
    if (auto error = match_wav_format(wav.format, wav.bits_per_sample, &sample_format)) {
        return error;
    }
    *out = {
        .gpa = nullptr,
        .sample_layout = AUSampleLayout_Interlaced,
        .sample_format = sample_format,
        .sample_rate = wav.sample_rate,
        .channel_count = wav.channel_count,
        .frame_count = wav.frame_count,
        .samples = {
            .i8 = wav.samples.i8,
        },
    };
    return e_au_decode_none;
}

e_au_decode au_audio_decode(Allocator* gpa, AUFormat format, Bytes bytes, AUAudio* out)
{
    AUWAV wav;
    switch (format) {
    case AUFormat_WAV:
        if (auto error = decode_wav(bytes, &wav)) {
            return error;
        }
    }
    AUAudio audio;
    if (auto error = borrow_wav(wav, &audio)) {
        return error;
    }
    (void)gpa;
    usize byte_size = au_audio_byte_size(&audio);
    auto* samples = (i8*)gpa->raw_alloc(byte_size, 16).or_default(nullptr);
    if (!samples) {
        return e_au_decode_out_of_memory;
    }
    audio.gpa = gpa;
    memcpy(samples, audio.samples.i8, byte_size);
    audio.samples.i8 = samples;
    *out = audio;
    return e_au_decode_none;
}

e_au_decode au_audio_decode_into_format(Schedule* schedule, Allocator* gpa, AUFormat format, Bytes bytes, AUAudioSpec spec, AUAudio* out)
{
    AUWAV wav;
    switch (format) {
    case AUFormat_WAV:
        if (auto error = decode_wav(bytes, &wav)) {
            return error;
        }
    }
    AUAudio audio;
    if (auto error = borrow_wav(wav, &audio)) {
        return error;
    }
    if (auto error = au_transcode(schedule, gpa, audio, spec, out)) {
        switch (error) {
        case e_au_transcode_none: return e_au_decode_none;
        case e_au_transcode_out_of_memory: return e_au_decode_out_of_memory;
        }
    }
    return e_au_decode_none;
}

Optional<AUFormat> au_format_guess(Bytes bytes)
{
    AUFormat format;
    if (au_format_guess(bytes, &format) != e_au_format_guess_none) {
        return {};
    }
    return format;
}

ErrorOr<AUAudioRef> AUAudioRef::decode(Allocator* gpa, AUFormat format, Bytes bytes)
{
    AUAudio audio;
    if (auto error = au_audio_decode(gpa, format, bytes, &audio)) {
        return Error::from_enum(error);
    }
    return AUAudioRef {
        .raw = audio,
    };
}

ErrorOr<AUAudioRef> AUAudioRef::decode_into_format(ScheduleRef schedule, Allocator* gpa, AUFormat format, Bytes bytes, AUAudioSpec to_spec)
{
    AUAudio audio;
    if (auto error = au_audio_decode_into_format(schedule.handle, gpa, format, bytes, to_spec, &audio)) {
        return Error::from_enum(error);
    }
    return AUAudioRef {
        .raw = audio,
    };
}

void AUAudioRef::destroy()
{
    au_audio_destroy(&raw);
}

ErrorOr<AUAudioRef> AUAudioRef::transcode(ScheduleRef schedule, Allocator* gpa, AUAudioSpec spec) const
{
    AUAudio audio;
    if (auto error = au_transcode(schedule.handle, gpa, raw, spec, &audio)) {
        return Error::from_enum(error);
    }
    return AUAudioRef {
        .raw = audio,
    };
}

i8 AUAudioRef::sample_i8(u32 channel, u64 frame) const
{
    return au_audio_sample_i8(&raw, channel, frame);
}

i16 AUAudioRef::sample_i16(u32 channel, u64 frame) const
{
    return au_audio_sample_i16(&raw, channel, frame);
}

i32 AUAudioRef::sample_i32(u32 channel, u64 frame) const
{
    return au_audio_sample_i32(&raw, channel, frame);
}

i64 AUAudioRef::sample_i64(u32 channel, u64 frame) const
{
    return au_audio_sample_i64(&raw, channel, frame);
}

f32 AUAudioRef::sample_f32(u32 channel, u64 frame) const
{
    return au_audio_sample_f32(&raw, channel, frame);
}

f64 AUAudioRef::sample_f64(u32 channel, u64 frame) const
{
    return au_audio_sample_f64(&raw, channel, frame);
}

f64 AUAudioRef::duration() const
{
    return au_audio_duration(&raw);
}

ErrorOr<AUAudioRef> au_transcode(ScheduleRef schedule, Allocator* gpa, AUAudioRef audio, AUAudioSpec to_spec)
{
    AUAudio output;
    if (auto error = au_transcode(schedule.handle, gpa, audio.raw, to_spec, &output)) {
        return Error::from_enum(error);
    }
    return AUAudioRef {
        .raw = output,
    };
}

static e_au_decode decode_wav(Bytes bytes, AUWAV* out)
{
    auto decoder = ByteDecoder(bytes);
    if (!decoder.expect("RIFF")) {
        return e_au_decode_wav_invalid_magic;
    }
    auto file_size = TRY(decoder.parse_u32le().or_throw([]{
        return e_au_decode_wav_could_not_decode_file_size;
    }));
    (void)file_size;
    if (!decoder.expect("WAVE")) {
        return e_au_decode_wav_no_wave_chunk;
    }
    if (!decoder.expect("fmt ")) {
        return e_au_decode_wav_no_fmt_chunk;
    }
    auto format_block_size = TRY(decoder.parse_u32le().or_throw([]{
        return e_au_decode_wav_could_not_decode_fmt_block_size;
    }));
    auto format_block = TRY(decoder.parse_bytes(format_block_size).or_throw([]{
        return e_au_decode_wav_fmt_block_size_mismatch;
    }));
    auto format_parser = ByteDecoder(format_block);
    auto format = TRY(format_parser.parse_u16le().map([](u16 format) {
        switch (format) {
        case 1: return AUWAVFormat_PCM;
        case 3: return AUWAVFormat_Float;
        }
        return AUWAVFormat_Unknown;
    }).or_throw([]{
        return e_au_decode_wav_invalid_audio_format;
    }));

    auto channel_count = TRY(format_parser.parse_u16le().or_throw([]{
        return e_au_decode_wav_could_not_decode_channel_count;
    }));

    auto sample_rate = TRY(format_parser.parse_u32le().or_throw([]{
        return e_au_decode_wav_could_not_decode_sample_rate;
    }));

    auto bytes_per_second = TRY(format_parser.parse_u32le().or_throw([]{
        return e_au_decode_wav_could_not_decode_sample_rate;
    }));

    auto bytes_per_block = TRY(format_parser.parse_u16le().or_throw([]{
        return e_au_decode_wav_could_not_decode_bytes_per_block;
    }));

    auto bits_per_sample = TRY(format_parser.parse_u16le().or_throw([]{
        return e_au_decode_wav_could_not_decode_bits_per_sample;
    }));

    auto samples = Bytes();
    while (decoder.peeks(4).has_value()) {
        auto section_name = decoder.parse_string(4);
        u32 data_size = TRY(decoder.parse_u32le().or_throw([]{
            return e_au_decode_wav_could_not_decode_section_size;
        }));
        if (section_name != "data") {
            decoder.skip(data_size);
            continue;
        }

        samples = TRY(decoder.parse_bytes(data_size).or_throw([]{
            return e_au_decode_wav_data_section_size_mismatch;
        }));
    }

    *out = {
        .format = format,
        .channel_count = channel_count,
        .sample_rate = sample_rate,
        .bytes_per_second = bytes_per_second,
        .bytes_per_block = bytes_per_block,
        .bits_per_sample = bits_per_sample,
        .frame_count = ((u32)samples.size()) / (bits_per_sample / 8) / channel_count,
        .samples = {
            .i8 = (i8*)samples.data(),
        }
    };
    return e_au_decode_none;
}

e_au_format_guess au_format_guess(Bytes bytes, AUFormat* format)
{
    AUWAV wav;
    if (decode_wav(bytes, &wav) != e_au_decode_none) {
        return e_au_format_guess_unknown_format;
    }
    return *format = AUFormat_WAV, e_au_format_guess_none;
}

c_string au_format_guess_strerror(e_au_format_guess error)
{
    switch (error) {
    case e_au_format_guess_none: return "no error";
    case e_au_format_guess_unknown_format: return "unknown format";
    }
}

c_string au_transcode_strerror(e_au_transcode error)
{
    switch (error) {
    case e_au_transcode_none: return "no error";
    case e_au_transcode_out_of_memory: return "out of memory";
    }
}

c_string au_decode_strerror(e_au_decode error)
{
    switch (error) {
    case e_au_decode_none: return "no error";
    case e_au_decode_out_of_memory: return "out of memory";
    case e_au_decode_unexpected_format: return "unexpected format";
    case e_au_decode_unsupported_invariant: return "unsupported invariant";

    case e_au_decode_wav_invalid_magic:                    return "invalid RIFF magic";
    case e_au_decode_wav_could_not_decode_file_size:       return "could not decode file size";
    case e_au_decode_wav_no_wave_chunk:                    return "missing WAVE chunk";
    case e_au_decode_wav_no_fmt_chunk:                     return "missing fmt chunk";
    case e_au_decode_wav_could_not_decode_fmt_block_size:  return "could not decode format block size";
    case e_au_decode_wav_fmt_block_size_mismatch:          return "format block size did not match what was expected";
    case e_au_decode_wav_invalid_audio_format:             return "could not parse audio format";
    case e_au_decode_wav_could_not_decode_channel_count:   return "could not decode channel count";
    case e_au_decode_wav_could_not_decode_sample_rate:     return "could not decode sample rate";
    case e_au_decode_wav_could_not_decode_bytes_per_block: return "could not decode bytes per second";
    case e_au_decode_wav_could_not_decode_bits_per_sample: return "could not decode bytes per block";
    case e_au_decode_wav_could_not_decode_section_size:    return "could not decode bits per sample";
    case e_au_decode_wav_section_size_mismatch:            return "section size did not match what was found";
    case e_au_decode_wav_data_section_size_mismatch:       return "data size did not match what was expected";
    }
}

c_string to_c_string(e_au_transcode error)
{
    return au_transcode_strerror(error);
}

c_string to_c_string(e_au_decode error)
{
    return au_decode_strerror(error);
}
