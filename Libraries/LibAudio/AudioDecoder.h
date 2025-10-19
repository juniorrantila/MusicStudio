#pragma once
#include <Basic/Base.h>
#include <Basic/Allocator.h>
#include <Basic/ByteDecoder.h>

typedef enum AUFormat {
    AUFormat_WAV,
} AUFormat;

typedef enum AUChannelLayout : u8 {
    AUSampleLayout_Interlaced = 0,
    AUSampleLayout_Linear = 1,
} AUSampleLayout;

typedef enum AUSampleKind : u8 {
    AUSampleKind_PCM = 0,
    AUSampleKind_Float = 1,
} AUSampleKind;

typedef enum AUSampleFormat : u8 {
    AUSampleFormat_I8  = (sizeof(i8)  << 1) | AUSampleKind_PCM,
    AUSampleFormat_I16 = (sizeof(i16) << 1) | AUSampleKind_PCM,
    AUSampleFormat_I32 = (sizeof(i32) << 1) | AUSampleKind_PCM,
    AUSampleFormat_I64 = (sizeof(i64) << 1) | AUSampleKind_PCM,
    AUSampleFormat_F32 = (sizeof(f32) << 1) | AUSampleKind_Float,
    AUSampleFormat_F64 = (sizeof(f64) << 1) | AUSampleKind_Float,
} AUSampleFormat;

typedef enum : u16 {
    e_au_format_guess_none = 0,
    e_au_format_guess_unknown_format,
} e_au_format_guess;
C_API c_string au_format_guess_strerror(e_au_format_guess);

typedef enum : u16 {
    e_au_transcode_none = 0,
    e_au_transcode_out_of_memory,
} e_au_transcode;
C_API c_string au_transcode_strerror(e_au_transcode);

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
C_API c_string au_decode_strerror(e_au_decode);

typedef struct AUAudio {
    Allocator* gpa;
    u64 frame_count;
    union {
        i8*  i8;
        i16* i16;
        i32* i32;
        i64* i64;
        f32* f32;
        f64* f64;
    } samples;
    u32 sample_rate;
    u16 channel_count;
    AUSampleLayout sample_layout;
    AUSampleFormat sample_format;

#if __cplusplus
    void destroy();

    u64 sample_count() const;
    u64 bytes_per_sample() const;
    u64 byte_size() const;

    i8 sample_i8(u64 channel, u64 frame) const;
    i16 sample_i16(u64 channel, u64 frame) const;
    i32 sample_i32(u64 channel, u64 frame) const;
    i64 sample_i64(u64 channel, u64 frame) const;
    f32 sample_f32(u64 channel, u64 frame) const;
    f64 sample_f64(u64 channel, u64 frame) const;

    f64 duration() const;
#endif
} AUAudio;
static_assert(sizeof(AUAudio) == 32);

typedef struct AUAudioSpec {
    AUSampleLayout sample_layout;
    AUSampleFormat sample_format;
    u32 sample_rate;
} AUAudioSpec;

C_API e_au_format_guess au_format_guess(Bytes bytes, AUFormat* format);

C_API e_au_decode au_audio_decode_wav(Bytes bytes, AUAudio*);
C_API e_au_decode au_audio_decode(Allocator* gpa, AUFormat format, Bytes bytes, AUAudio*);
C_API e_au_decode au_audio_decode_into_format(Allocator* gpa, AUFormat format, Bytes bytes, AUAudioSpec spec, AUAudio*);
C_API void au_audio_destroy(AUAudio*);

C_API u64 au_audio_sample_count(AUAudio const*);
C_API u64 au_audio_bytes_per_sample(AUAudio const*);
C_API u64 au_audio_byte_size(AUAudio const*);

C_API i8 au_audio_sample_i8(AUAudio const* audio, u64 channel, u64 frame);
C_API i16 au_audio_sample_i16(AUAudio const* audio, u64 channel, u64 frame);
C_API i32 au_audio_sample_i32(AUAudio const* audio, u64 channel, u64 frame);
C_API i64 au_audio_sample_i64(AUAudio const* audio, u64 channel, u64 frame);
C_API f32 au_audio_sample_f32(AUAudio const* audio, u64 channel, u64 frame);
C_API f64 au_audio_sample_f64(AUAudio const* audio, u64 channel, u64 frame);
C_API f64 au_audio_duration(AUAudio const* audio);

C_API e_au_transcode au_transcode(Allocator* gpa, AUAudio input, AUAudioSpec to_spec, AUAudio*);
