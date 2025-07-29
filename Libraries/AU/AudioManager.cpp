#include "./AudioManager.h"

#include "./AudioDecoder.h"

#include <Ty/StringSlice.h>
#include <Ty2/Base.h>

#include <Ty2/Allocator.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/Verify.h>
#include <Ty2/Hash.h>

#include <FS/FSVolume.h>

#include <Core/Time.h>

#include <SoundIo/SoundIo.h>
#include <pthread.h>
#include <string.h>

static_assert(au_audio_channel_max <= SOUNDIO_MAX_CHANNELS);

static void* io_thread(void*);
static u16 path_slot(AUAudioID);
static u16 block_slot(AUAudioBlockID);
static bool did_just_prefetch(AUAudioManager*, AUAudioBlockID);
static bool block_equal(AUAudioBlockID a, AUAudioBlockID b);
static void prefetch_history_push(AUAudioManager*, AUAudioBlockID);

ty_define_message(AUAudioManagerOpen) {
    AUAudioID id;
    char path[au_audio_file_path_max];
};
static_assert(sizeof(AUAudioManagerOpen) <= message_size_max);

ty_define_message(AUAudioManagerPrepare) {
   AUAudioBlockID id;
   AUAudioBlock* block;
   char path[au_audio_file_path_max];
};
static_assert(sizeof(AUAudioManagerPrepare) <= message_size_max);

C_API [[nodiscard]] bool au_audio_manager_init(AUAudioManager* audio, Logger* io_debug)
{
    memzero(audio, sizeof(*audio));
    mempoison(audio->blocks, sizeof(audio->blocks));

    audio->io_debug = io_debug;
    fs_volume_init(&audio->volume);
    audio->volume.debug = audio->io_debug;
    audio->volume.automount_when_not_found = true;

    if (!memory_poker_init(&audio->memory_poker))
        return false;
    audio->memory_poker.push(audio, sizeof(*audio));
    if (!mailbox_init(sizeof(audio->blocks), &audio->io_mailbox).ok)
        return false;
    audio->io_mailbox.attach_memory_poker(&audio->memory_poker);
    if (pthread_create(&audio->io_thread, nullptr, io_thread, audio) != 0)
        return false;

    return true;
}

AUAudioID AUAudioManager::audio(StringSlice file_name) { return au_audio_id(this, file_name); }
C_API AUAudioID au_audio_id(AUAudioManager* audio, StringSlice file_name)
{
    VERIFY(file_name.count < PATH_MAX);
    VERIFY(file_name.count > 0);

    u64 hash = djb2(djb2_initial_seed, file_name.items, file_name.count);
    if (hash == au_audio_id_null.hash) hash += 1;
    auto id = (AUAudioID){hash};

    auto* slot = &audio->audios[path_slot(id)];
    if (slot->id.hash == id.hash)
        return id;
    if (memcmp(audio->paths[path_slot(id)], file_name.items, file_name.count) == 0)
        return au_audio_id_null; // Not ready.

    memcpy(audio->paths[path_slot(id)], file_name.items, file_name.count);
    auto open = (AUAudioManagerOpen){
        .id = id,
        .path = {},
    };
    memcpy(open.path, file_name.items, file_name.count);
    if (!audio->io_mailbox.writer()->post(open).ok)
        return au_audio_id_null;
    return id;
}

AUAudioBlockID AUAudioManager::block(AUAudioID id, u64 frame, u16 channel) { return au_audio_block_id(id, frame, channel); }
C_API AUAudioBlockID au_audio_block_id(AUAudioID audio, u64 frame, u16 channel)
{
    VERIFY(channel < au_audio_channel_max);
    return (AUAudioBlockID){
        .audio_id = audio,
        .block = frame / au_audio_frames_per_block,
        .channel = (u8)channel,
    };
}

void AUAudioManager::prefetch(AUAudioID id, i64 frame, u16 channel) { return au_audio_prefetch(this, id, frame, channel); }
C_API void au_audio_prefetch(AUAudioManager* audio, AUAudioID id, i64 frame, u16 channel)
{
    if (id.hash == au_audio_id_null.hash)
        return;
    if (frame < 0) frame = 0;

    auto block_id = au_audio_block_id(id, frame, channel);
    auto* block = &audio->blocks[block_slot(block_id)];
    memunpoison(block, sizeof(*block));
    if (block_equal(block_id, block->id))
        return;
    if (did_just_prefetch(audio, block_id))
        return;

    char* path = audio->paths[path_slot(id)];
    auto prepare = (AUAudioManagerPrepare){
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
    VERIFY(channel < au_audio_channel_max);
    if (id.hash == au_audio_id_null.hash)
        return 0; // Not ready.

    au_audio_prefetch(audio, id, frame + 0 * au_audio_frames_per_block, channel);
    au_audio_prefetch(audio, id, frame + 1 * au_audio_frames_per_block, channel);
    if (frame < 0)
        return 0;

    auto block_id = au_audio_block_id(id, frame, channel);
    auto slot = block_slot(block_id);
    auto* block = &audio->blocks[slot];
    if (!block_equal(block->id, block_id))
        return 0; // Not ready
    u64 sample_slot = ((u64)frame) % au_audio_frames_per_block;
    return block->samples[sample_slot];
}

static u16 block_slot(AUAudioBlockID block)
{
    return djb2(djb2_initial_seed, &block, sizeof(block)) % au_audio_block_max;
}

static u16 path_slot(AUAudioID id)
{
    return id.hash % au_audio_file_max;
}

 static void* io_thread(void* user)
{
    pthread_setname_np("audio-manager");

    auto* audio = (AUAudioManager*)user;
    auto* log = audio->io_debug;

    for (;;) {
        audio->io_mailbox.reader()->wait();

        u16 tag = 0;
        while (audio->io_mailbox.reader()->peek(&tag).found) {
            switch (tag) {
            case Ty2::type_id<AUAudioManagerOpen>(): {
                AUAudioManagerOpen open;
                VERIFY(audio->io_mailbox.reader()->read(&open).ok);
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
                if (auto error = au_audio_decode_wav(bytes(content.items, content.count), &slot->audio); error != e_au_decode_none) {
                    log->error("could not decode '%s': %s", open.path, au_decode_strerror(error));
                    continue;
                }

                ty_write_barrier();
                slot->id = open.id;
                ty_write_barrier();

                log->info("opened '%s'", open.path);

                continue;
            }
            case Ty2::type_id<AUAudioManagerPrepare>(): {
                AUAudioManagerPrepare prepare;
                VERIFY(audio->io_mailbox.reader()->read(&prepare).ok);

                auto path = string_slice_from_c_string(prepare.path);
                FileID id;
                if (!fs_volume_find(&audio->volume, path, &id)) {
                    // Could not open file.
                    log->error("could not open '%s' (%zu)", prepare.path, prepare.id.block);
                    continue;
                }

                auto* slot = &audio->audios[path_slot(prepare.id.audio_id)];
                auto* file = fs_volume_use_ref(&audio->volume, id);
                if (fs_file_needs_reload(file)) {
                    fs_file_reload(file);
                    auto content = fs_content(*file);

                    if (auto error = au_audio_decode_wav(bytes(content.items, content.count), &slot->audio); error != e_au_decode_none) {
                        log->error("could not decode '%s': %s", prepare.path, au_decode_strerror(error));
                        continue;
                    }
                }
                VERIFY(slot->audio.channel_count <= au_audio_channel_max);
                auto* block = prepare.block;
                auto old_block = *block;

                u64 sample_start = prepare.id.block * au_audio_frames_per_block;
                u64 sample_end = sample_start + au_audio_frames_per_block;
                u64 relative_frame = 0;
                u16 channel = prepare.id.channel;
                for (u64 absolute_frame = sample_start; absolute_frame < sample_end; absolute_frame++, relative_frame++) {
                    block->samples[relative_frame] = au_audio_sample_f64(&slot->audio, channel, absolute_frame);
                }
                ty_write_barrier();
                block->id = prepare.id;
                ty_write_barrier();

                if (old_block.id.audio_id.hash != au_audio_id_null.hash) {
                    c_string old_name = audio->paths[path_slot(old_block.id.audio_id)];
                    log->debug("evicted slot %.5u (%s:%.5zu:%.2u => %s:%.5zu:%.2u)",
                        block_slot(prepare.id),
                        old_name, old_block.id.block, old_block.id.channel,
                        prepare.path, prepare.id.block, prepare.id.channel
                    );
                }
                continue;
            }
            default: log->fatal("unhandled message: %s", ty_type_name(tag));
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

