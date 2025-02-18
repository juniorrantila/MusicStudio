#pragma once
#include <Ty/Base.h>
#include <Ty/Allocator.h>

#include <Schedule/Schedule.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AUFormat {
    AUFormat_WAV,
} AUFormat;

typedef enum AUChannelLayout : u16 {
    AUSampleLayout_Interlaced,
    AUSampleLayout_Linear,
} AUSampleLayout;

typedef enum AUSampleKind : u8 {
    AUSampleKind_PCM = 0,
    AUSampleKind_Float = 1,
} AUSampleKind;

typedef enum AUSampleFormat : u16 {
    AUSampleFormat_I8  = (sizeof(i8)  << 1) | AUSampleKind_PCM,
    AUSampleFormat_I16 = (sizeof(i16) << 1) | AUSampleKind_PCM,
    AUSampleFormat_I32 = (sizeof(i32) << 1) | AUSampleKind_PCM,
    AUSampleFormat_I64 = (sizeof(i64) << 1) | AUSampleKind_PCM,
    AUSampleFormat_F16 = (sizeof(f16) << 1) | AUSampleKind_Float,
    AUSampleFormat_F32 = (sizeof(f32) << 1) | AUSampleKind_Float,
    AUSampleFormat_F64 = (sizeof(f64) << 1) | AUSampleKind_Float,
} AUSampleFormat;

typedef enum : u16 {
    e_au_format_guess_none = 0,
    e_au_format_guess_unknown_format,
} e_au_format_guess;
c_string au_format_guess_strerror(e_au_format_guess);

typedef enum : u16 {
    e_au_transcode_none = 0,
    e_au_transcode_out_of_memory,
} e_au_transcode;
c_string au_transcode_strerror(e_au_transcode);

typedef enum : u16 {
    e_au_decode_none = 0,
    e_au_decode_out_of_memory,
    e_au_decode_unexpected_format,
    e_au_decode_unsupported_invariant,

    e_au_decode_wav_invalid_magic,
    e_au_decode_wav_could_not_decode_file_size,
    e_au_decode_wav_no_wave_chunk,
    e_au_decode_wav_no_fmt_chunk,
    e_au_decode_wav_could_not_decode_fmt_block_size,
    e_au_decode_wav_fmt_block_size_mismatch,
    e_au_decode_wav_invalid_audio_format,
    e_au_decode_wav_could_not_decode_channel_count,
    e_au_decode_wav_could_not_decode_sample_rate,
    e_au_decode_wav_could_not_decode_bytes_per_block,
    e_au_decode_wav_could_not_decode_bits_per_sample,
    e_au_decode_wav_could_not_decode_section_size,
    e_au_decode_wav_section_size_mismatch,
    e_au_decode_wav_data_section_size_mismatch,

} e_au_decode;
c_string au_decode_strerror(e_au_decode);

typedef struct AUAudio {
    Allocator* gpa;
    AUSampleLayout sample_layout;
    AUSampleFormat sample_format;
    u32 sample_rate;
    u32 channel_count;
    u64 frame_count;
    union {
        i8*  i8;
        i16* i16;
        i32* i32;
        i64* i64;
        f16* f16;
        f32* f32;
        f64* f64;
    } samples;
} AUAudio;

typedef struct AUAudioSpec {
    AUSampleLayout sample_layout;
    AUSampleFormat sample_format;
    u32 sample_rate;
} AUAudioSpec;

e_au_format_guess au_format_guess(Bytes bytes, AUFormat* format);

e_au_decode au_audio_decode(Allocator* gpa, AUFormat format, Bytes bytes, AUAudio*);
e_au_decode au_audio_decode_into_format(Schedule* schedule, Allocator* gpa, AUFormat format, Bytes bytes, AUAudioSpec spec, AUAudio*);
void au_audio_destroy(AUAudio*);

u64 au_audio_sample_count(AUAudio const*);
u64 au_audio_bytes_per_sample(AUAudio const*);
u64 au_audio_byte_size(AUAudio const*);

i8 au_audio_sample_i8(AUAudio const* audio, usize channel, usize frame);
i16 au_audio_sample_i16(AUAudio const* audio, usize channel, usize frame);
i32 au_audio_sample_i32(AUAudio const* audio, usize channel, usize frame);
i64 au_audio_sample_i64(AUAudio const* audio, usize channel, usize frame);
f16 au_audio_sample_f16(AUAudio const* audio, usize channel, usize frame);
f32 au_audio_sample_f32(AUAudio const* audio, usize channel, usize frame);
f64 au_audio_sample_f64(AUAudio const* audio, usize channel, usize frame);
f64 au_audio_duration(AUAudio const* audio);

e_au_transcode au_transcode(Schedule* schedule, Allocator* gpa, AUAudio input, AUAudioSpec to_spec, AUAudio*);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

Optional<AUFormat> au_format_guess(Bytes bytes);

struct AUAudioRef {
    AUAudio raw {};

    static ErrorOr<AUAudioRef> decode(Allocator* gpa, AUFormat format, Bytes bytes);
    static ErrorOr<AUAudioRef> decode_into_format(ScheduleRef schedule, Allocator* gpa, AUFormat format, Bytes bytes, AUAudioSpec to_spec);

    void destroy();

    ErrorOr<AUAudioRef> transcode(ScheduleRef schedule, Allocator* gpa, AUAudioSpec spec) const;

    i8 sample_i8(u32 channel, u64 frame) const;
    i16 sample_i16(u32 channel, u64 frame) const;
    i32 sample_i32(u32 channel, u64 frame) const;
    i64 sample_i64(u32 channel, u64 frame) const;
    f16 sample_f16(u32 channel, u64 frame) const;
    f32 sample_f32(u32 channel, u64 frame) const;
    f64 sample_f64(u32 channel, u64 frame) const;

    f64 duration() const;
};

ErrorOr<AUAudioRef> au_transcode(ScheduleRef schedule, Allocator* gpa, AUAudioRef audio, AUAudioSpec to_spec);

c_string to_c_string(e_au_transcode error);
c_string to_c_string(e_au_decode error);

#endif
