#pragma once
#include "AudioDecoder.h"

#include <Ty2/MemoryPoker.h>
#include <Ty2/Base.h>
#include <Ty2/Mailbox.h>
#include <Ty/StringSlice.h>
#include <FS/FSVolume.h>

#include <pthread.h>
#include <sys/syslimits.h>

constexpr i64 au_audio_frames_per_block = 1024;
constexpr u64 au_audio_block_max = 1024;
constexpr u64 au_audio_block_channel_max = 24;
constexpr u64 au_audio_file_max = OPEN_MAX;

constexpr u64 au_audio_file_path_max = PATH_MAX;
static_assert(au_audio_file_path_max <= PATH_MAX);
static_assert(au_audio_file_path_max <= message_size_max);

typedef struct { u64 hash; } AUAudioID;
constexpr AUAudioID au_audio_id_null = { 0 };

typedef struct {
    AUAudioID audio_id;
    u64 block;
} AUAudioBlockID;

typedef struct {
    f64 samples[au_audio_block_channel_max][au_audio_frames_per_block];
    AUAudioBlockID id; // FIXME: Make this atomic
} AUAudioBlock;


typedef struct AUAudioManager {
    AUAudioBlock blocks[au_audio_block_max]; // Accessed via djb2(block_id) % au_audio_block_max
    char paths[au_audio_file_max][au_audio_file_path_max]; // Accessed via djb2(file_name) % au_audio_file_max

    Mailbox io_mailbox;
    pthread_t io_thread;

    struct {
        u32 head;
        AUAudioBlockID buffer[256];
    } prefetch_history;

    Logger* io_debug;
    FSVolume volume;

    struct {
        AUAudioID id;
        AUAudio audio;
    } audios[au_audio_block_max];

    MemoryPoker memory_poker;

#if __cplusplus
    AUAudioID audio(StringSlice file_name);
    static AUAudioBlockID block(AUAudioID, u64 frame);

    void prefetch(AUAudioID, i64 frame);
    f64 sample(AUAudioID, i64 frame, u16 channel);
#endif
} AUAudioManager;

C_API [[nodiscard]] bool au_audio_manager_init(AUAudioManager*, Logger* io_debug);

C_API AUAudioID au_audio_id(AUAudioManager*, StringSlice file_name);
C_API AUAudioBlockID au_audio_block_id(AUAudioID audio, u64 frame);

C_API void au_audio_prefetch(AUAudioManager*, AUAudioID, i64 frame);
C_API f64 au_audio_sample(AUAudioManager*, AUAudioID, i64 frame, u16 channel);
