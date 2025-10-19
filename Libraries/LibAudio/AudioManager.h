#pragma once
#include "./AudioDecoder.h"

#include <Basic/Bits.h>
#include <Basic/MemoryPoker.h>
#include <Basic/Base.h>
#include <Basic/Mailbox.h>
#include <Basic/StringSlice.h>

#include <LibCore/FSVolume.h>

#include <pthread.h>
#include <sys/syslimits.h>

constexpr i64 au_audio_frames_per_block = 512;
constexpr u64 au_audio_block_max = 16384;
constexpr u64 au_audio_channel_max = 24;
constexpr u64 au_audio_file_max = OPEN_MAX;

constexpr u64 au_audio_file_path_max = PATH_MAX;
static_assert(au_audio_file_path_max <= PATH_MAX);
static_assert(au_audio_file_path_max <= message_size_max);

typedef struct { u64 hash; } AUAudioID;
constexpr AUAudioID au_audio_id_null = { 0 };

typedef struct {
    AUAudioID audio_id;
    u64 block : 60;
    u8 channel : 4; static_assert(au_audio_channel_max < ty_bituint_max(4));
} AUAudioBlockID;

typedef struct {
    f64 samples[au_audio_frames_per_block];
    AUAudioBlockID id; // FIXME: Make this atomic
} AUAudioBlock;


typedef struct AUAudioManager {
    AUAudioBlock blocks[au_audio_block_max];
    char paths[au_audio_file_max][au_audio_file_path_max];

    Mailbox io_mailbox;
    pthread_t io_thread;

    struct {
        u32 head;
        AUAudioBlockID buffer[256];
    } prefetch_history;

    FSVolume volume;

    struct {
        AUAudioID id;
        AUAudio audio;
    } audios[au_audio_file_max];

#if __cplusplus
    AUAudioID audio(StringSlice file_name);
    static AUAudioBlockID block(AUAudioID, u64 frame, u16 channel);

    void prefetch(AUAudioID, i64 frame, u16 channel);
    f64 sample(AUAudioID, i64 frame, u16 channel);
#endif
} AUAudioManager;
static_assert(sizeof(AUAudioManager) <= 96 * MiB);

C_API [[nodiscard]] bool au_audio_manager_init(AUAudioManager*, MemoryPoker* poker);
C_API [[nodiscard]] bool au_audio_manager_start(AUAudioManager*);

C_API AUAudioID au_audio_id(AUAudioManager*, StringSlice file_name);
C_API AUAudioBlockID au_audio_block_id(AUAudioID audio, u64 frame, u16 channel);

C_API void au_audio_prefetch(AUAudioManager*, AUAudioID, i64 frame, u16 channel);
C_API f64 au_audio_sample(AUAudioManager*, AUAudioID, i64 frame, u16 channel);
