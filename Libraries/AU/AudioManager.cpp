#include "./AudioManager.h"

#include "./AudioDecoder.h"

#include <Ty/StringSlice.h>
#include <Ty2/Base.h>

#include <Ty2/Allocator.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/Verify.h>
#include <Ty2/Hash.h>

#include <FS/FSVolume.h>

#include <SoundIo/SoundIo.h>
#include <pthread.h>
#include <string.h>

static_assert(au_audio_block_channel_max == SOUNDIO_MAX_CHANNELS);

static void* io_thread(void*);
static u16 path_slot(AUAudioID);
static u16 block_slot(AUAudioBlockID);
static bool did_just_prefetch(AUAudioManager*, AUAudioBlockID);
static bool block_equal(AUAudioBlockID a, AUAudioBlockID b);
static void prefetch_history_push(AUAudioManager*, AUAudioBlockID);

struct Open {
    AUAudioID id;
    char path[PATH_MAX];
};

struct Prepare {
   AUAudioBlockID id;
   AUAudioBlock* block;
   char path[PATH_MAX];
};

C_API [[nodiscard]] bool au_audio_manager_init(AUAudioManager* audio, Logger* io_debug)
{
    memzero(audio, sizeof(*audio));

    audio->io_debug = io_debug;
    fs_volume_init(&audio->volume);
    audio->volume.debug = audio->io_debug;
    audio->volume.automount_when_not_found = true;

    if (!mailbox_init(2 * au_audio_block_max, &audio->io_mailbox).ok)
        return false;
    if (pthread_create(&audio->io_thread, nullptr, io_thread, audio) != 0)
        return false;
    return true;
}

AUAudioID AUAudioManager::audio(StringSlice file_name) { return au_audio_id(this, file_name); }
C_API AUAudioID au_audio_id(AUAudioManager* audio, StringSlice file_name)
{
    VERIFY(file_name.count < PATH_MAX);

    u64 hash = djb2(djb2_initial_seed, file_name.items, file_name.count);
    if (hash == au_audio_id_null.hash) hash += 1;
    auto id = (AUAudioID){hash};

    char* path = audio->paths[path_slot(id)];
    if (memcmp(path, file_name.items, file_name.count) == 0)
        return id;

    memcpy(path, file_name.items, file_name.count);
    auto open = Open{
        .id = id,
        .path = {},
    };
    memcpy(open.path, file_name.items, file_name.count);
    if (!audio->io_mailbox.writer()->post(open).ok)
        return au_audio_id_null;

    return id;
}

AUAudioBlockID AUAudioManager::block(AUAudioID id, u64 frame) { return au_audio_block_id(id, frame); }
C_API AUAudioBlockID au_audio_block_id(AUAudioID audio, u64 frame)
{
    return (AUAudioBlockID){
        .audio_id = audio,
        .block = frame / au_audio_frames_per_block,
    };
}

void AUAudioManager::prefetch(AUAudioID id, i64 frame) { return au_audio_prefetch(this, id, frame); }
C_API void au_audio_prefetch(AUAudioManager* audio, AUAudioID id, i64 frame)
{
    if (id.hash == au_audio_id_null.hash)
        return;
    if (frame < 0) frame = 0;

    auto block_id = au_audio_block_id(id, frame);
    auto* block = &audio->blocks[block_slot(block_id)];
    if (block_equal(block_id, block->id))
        return;
    if (did_just_prefetch(audio, block_id))
        return;

    char* path = audio->paths[path_slot(id)];
    auto prepare = Prepare{
        .id = block_id,
        .block = block,
        .path = {},
    };
    memcpy(prepare.path, path, sizeof(audio->paths[0]));
    if (!audio->io_mailbox.writer()->post(prepare).ok)
        return;
    prefetch_history_push(audio, block_id);
}

f64 AUAudioManager::sample(AUAudioID id, i64 frame, u16 channel) { return au_audio_sample(this, id, frame, channel); }
C_API f64 au_audio_sample(AUAudioManager* audio, AUAudioID id, i64 frame, u16 channel)
{
    VERIFY(channel < au_audio_block_channel_max);

    au_audio_prefetch(audio, id, frame + au_audio_frames_per_block);
    if (frame < 0)
        return 0;

    // FIXME: Remove this.
    auto* a = &audio->audios[path_slot(id)];
    if (a->id.hash != id.hash)
        return 0;
    return au_audio_sample_f64(&a->audio, channel, frame);

    auto block_id = au_audio_block_id(id, frame);
    auto slot = block_slot(block_id);
    auto* block = &audio->blocks[slot];
    if (!block_equal(block->id, block_id))
        return 0; // Not ready
    u64 sample_slot = frame % au_audio_frames_per_block;
    return block->samples[channel][sample_slot];
}

static u16 block_slot(AUAudioBlockID block)
{
    return djb2_u64(block.audio_id.hash, block.block) % au_audio_block_max;
}

static u16 path_slot(AUAudioID id)
{
    return id.hash % au_audio_block_max;
}

 static void* io_thread(void* user)
{
    auto* audio = (AUAudioManager*)user;
    auto* log = audio->io_debug;
    // auto arena = fixed_arena_init(page_alloc(1 * GiB), 1 * GiB);
    // page_alloc(1 * GiB);

    for (;;) {
        audio->io_mailbox.reader()->wait();

        Message message;
        while (audio->io_mailbox.reader()->read(&message).found) {
            switch (message.tag) {
            case Ty2::type_id<Open>(): {
                Open open;
                VERIFY(message.unwrap(&open).ok);
                auto path = string_slice_from_c_string(open.path);
                FileID id;
                if (!fs_volume_find(&audio->volume, path, &id)) {
                    log->error("could not open '%s'", open.path);
                    continue;
                }

                auto* file = fs_volume_use_ref(&audio->volume, id);
                if (fs_file_needs_reload(file))
                    fs_file_reload(file);
                auto content = fs_content(*file);

                auto* slot = &audio->audios[path_slot(open.id)];
                if (au_audio_decode_wav(bytes(content.items, content.count), &slot->audio) != e_au_decode_none) {
                    log->error("could not decode '%s'", open.path);
                    continue;
                }

                ty_write_barrier();
                slot->id = open.id;

                continue;
            }
            case Ty2::type_id<Prepare>(): {
                Prepare prepare;
                VERIFY(message.unwrap(&prepare).ok);

                auto path = string_slice_from_c_string(prepare.path);
                FileID id;
                if (!fs_volume_find(&audio->volume, path, &id)) {
                    // Could not open file.
                    log->error("could not open '%s' (%zu)", prepare.path, prepare.id.block);
                    continue;
                }

                auto* file = fs_volume_use_ref(&audio->volume, id);
                if (fs_file_needs_reload(file))
                    fs_file_reload(file);
                auto content = fs_content(*file);

                AUAudio audio;
                if (au_audio_decode_wav(bytes(content.items, content.count), &audio) != e_au_decode_none) {
                    log->error("could not decode '%s' (%zu)", prepare.path, prepare.id.block);
                    continue;
                }
                VERIFY(audio.channel_count <= au_audio_block_channel_max);
                auto* block = prepare.block;
                u64 sample_start = prepare.id.block * au_audio_frames_per_block;
                u64 sample_end = sample_start + au_audio_frames_per_block - 1;
                for (u16 channel = 0; channel < audio.channel_count; channel++) {
                    for (u64 frame = sample_start; frame < sample_end; frame++) {
                        block->samples[channel][frame % au_audio_frames_per_block] = au_audio_sample_f64(&audio, channel, frame);
                    }
                }
                ty_write_barrier();
                block->id = prepare.id;
                ty_write_barrier();

                // log->info("prefetched '%s' (%zu)", prepare.path, prepare.id.block);
                continue;
            }
            default: UNREACHABLE();
            }
        }
    }

    UNREACHABLE();
    return nullptr;
}

static bool block_equal(AUAudioBlockID a, AUAudioBlockID b) { return memcmp(&a, &b, sizeof(a)) == 0; }

static void prefetch_history_push(AUAudioManager* audio, AUAudioBlockID id)
{
    auto* prefetch = &audio->prefetch_history;
    prefetch->head = (prefetch->head + 1) % ty_array_size(prefetch->buffer);
    prefetch->buffer[prefetch->head] = id;
}

static bool did_just_prefetch(AUAudioManager* audio, AUAudioBlockID id)
{
    for (u32 i = 0; i < ty_array_size(audio->prefetch_history.buffer); i++) {
        if (block_equal(id, audio->prefetch_history.buffer[i]))
            return true;
    }
    return false;
}

