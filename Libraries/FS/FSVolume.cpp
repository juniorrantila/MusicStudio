#include "./FSVolume.h"
#include "Ty/StringSlice.h"
#include "Ty2/FixedArena.h"

#include <Ty/Verify.h>
#include <Ty/Defer.h>
#include <Ty2/PageAllocator.h>

#include <Core/Time.h>

#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static bool expand_if_needed(FSVolume* volume);

C_API void fs_volume_init(FSVolume* volume)
{
    *volume = (FSVolume){};
    memset(volume, 0, sizeof(FSVolume));
    volume->watch_fd = -1;
    volume->event_arena = fixed_arena_init(volume->arena_store, sizeof(volume->arena_store));
}

Optional<Tar*> FSVolume::as_tar(Allocator* gpa) const { return fs_volume_as_tar(this, gpa); }
C_API Tar* fs_volume_as_tar(FSVolume const*, Allocator*)
{
    UNIMPLEMENTED();
}


static StringSlice const* file_content(FSFile const* file)
{
    switch (file->kind) {
    case FSFileMount_VirtualMount:
        return &file->virtual_mount.content;
    case FSFileMount_SystemMount:
        return &file->system_mount.content;
    }
}


Optional<StringSlice const*> FSVolume::open(StringSlice path) { return fs_volume_open(this, path); }
C_API StringSlice const* fs_volume_open(FSVolume* volume, StringSlice path)
{
    auto id = volume->find(path);
    if (!id.has_value()) {
        return nullptr;
    }
    return file_content(volume->use_ref(id.value()));
}


FSFile FSVolume::use(FileID id) const { return fs_volume_use(this, id); }
C_API FSFile fs_volume_use(FSVolume const* volume, FileID id)
{
    return *fs_volume_use_ref(volume, id);
}

FSFile* FSVolume::use_ref(FileID id) const { return fs_volume_use_ref(this, id); }
C_API FSFile* fs_volume_use_ref(FSVolume const* volume, FileID id)
{
    VERIFY(id.index < volume->count);
    return &volume->items[id.index];
}

static bool mount_no_cache(FSVolume* volume, FSFile file, FileID* out)
{
    if (!expand_if_needed(volume)) return false;
    usize id = volume->count++;
    volume->items[id] = file;
    if (out) {
        *out = (FileID){ id };
    }
    return true;
}

Optional<FileID> FSVolume::find(StringSlice path)
{
    FileID id;
    if (!fs_volume_find(this, path, &id))
        return {};
    return id;
}
C_API bool fs_volume_find(FSVolume* volume, StringSlice path, FileID* id)
{
    char path_buf[PATH_MAX];
    FixedArena arena = fixed_arena_init(path_buf, sizeof(path_buf));

    auto resolved_path = path.resolve_path(&arena.allocator);
    if (!resolved_path.has_value()) {
        return false;
    }
    for (usize i = 0; i < volume->count; i++) {
        auto other = fs_virtual_path(volume->items[i]);
        if (resolved_path->equal(other)) {
            if (id) *id = (FileID){ .index = i };
            return true;
        }
    }

    if (volume->automount_when_not_found) {
        auto* log = volume->debug;
        if (log) log->warning("could not find '%.*s', trying to auto mount it", (int)path.count, path.items);
        FSFile file;
        if (!fs_system_open(page_allocator(), path, &file)) {
            if (log) log->warning("could not open '%.*s'", (int)path.count, path.items);
            return false;
        }
        if (!mount_no_cache(volume, file, id)) {
            if (log) log->warning("could not mount '%.*s'", (int)path.count, path.items);
            return false;
        }
        return true;
    }

    return false;
}


Optional<FileID> FSVolume::mount(FSFile file)
{
    FileID id;
    if (!fs_volume_mount(this, file, &id)) {
        return {};
    }
    return id;
}


C_API bool fs_volume_mount(FSVolume* volume, FSFile file, FileID* out)
{
    auto path = fs_virtual_path(file);
    if (fs_volume_find(volume, path, out))
        return true;
    return mount_no_cache(volume, file, out);
}

static bool is_file_deleted(FSFile const* file)
{
    switch (file->kind) {
    case FSFileMount_VirtualMount:
        return false;
    case FSFileMount_SystemMount:
        return file->system_mount.deleted_at != 0.0;
    }
}

static struct kevent* alloc_kevent(FixedArena* arena, usize count)
{
    auto* events = (struct kevent*)arena->push(count * sizeof(struct kevent), alignof(struct kevent));
    if (events) memset(events, 0, count * sizeof(struct kevent));
    return events;
}

static usize system_file_count(FSVolume const* volume)
{
    usize count = 0;
    for (usize i = 0; i < volume->count; i++) {
        if (volume->items[i].kind == FSFileMount_SystemMount)
            count += 1;
    }
    return count;
}

static int fill_in_events(FSVolume const* volume, struct kevent* events, usize count)
{
    int event_id = 0;
    for (usize i = 0; i < volume->count; i++) {
        FSFile* file = &volume->items[i];
        if (file->kind == FSFileMount_SystemMount) {
            usize id = event_id++;
            VERIFY(id < count);
            events[id] = (struct kevent) {
                .ident = (uptr)file->system_mount.fd,
                .filter = EVFILT_VNODE,
                .flags = EV_ADD | EV_ONESHOT,
                .fflags = NOTE_WRITE | NOTE_DELETE,
                .data = 0,
                .udata = file,
            };
        }
    }
    return event_id;
}

static FSEvent* alloc_events(FixedArena* arena, usize count)
{
    auto* events = (FSEvent*)arena->push(count * sizeof(FSEvent), alignof(FSEvent));
    if (events) memset(events, 0, count * sizeof(FSEvent));
    return events;
}

static FSFile** reopen_deleted_files(FSVolume const* volume, FixedArena* arena, usize* count)
{
    usize deleted_files = 0;
    for (usize i = 0; i < volume->count; i++) {
        if (is_file_deleted(&volume->items[i])) deleted_files++;
    }
    if (deleted_files == 0) {
        return *count = 0, nullptr;
    }

    usize opened_count = 0;
    auto* files = (FSFile**)arena->push(deleted_files * sizeof(FSFile*), alignof(FSFile*));
    if (!files) return *count = 0, nullptr;

    for (usize i = 0; i < volume->count; i++) {
        auto* file = &volume->items[i];
        if (!is_file_deleted(file))
            continue;
        VERIFY(file->kind == FSFileMount_SystemMount);
        auto* mount = &file->system_mount;
        int fd = open(mount->path.items, O_RDONLY);
        if (fd < 0) continue;
        mount->fd = fd;
        mount->stale_at = mount->deleted_at;
        mount->deleted_at = 0.0;
        files[opened_count++] = file;
    }

    return *count = opened_count, files;
}

C_API FSEvents fs_volume_poll_events(FSVolume* volume, struct timespec const* timeout)
{
    if (volume->watch_fd < 0) {
        int fd = kqueue();
        if (fd < 0) return {};
        volume->watch_fd = fd;
    }
    auto* arena = &volume->event_arena;
    arena->drain();

    FSEvents* events = &volume->events;
    memset(events, 0, sizeof(*events));

    usize system_count = system_file_count(volume);
    auto* in_events = alloc_kevent(arena, system_count);
    if (!in_events) return {};
    auto* out_events = alloc_kevent(arena, system_count);
    if (!out_events) return {};

    int in_event_count = fill_in_events(volume, in_events, system_count);
    int kevent_count = kevent(volume->watch_fd, in_events, in_event_count, out_events, in_event_count, timeout);
    if (kevent_count < 0) {
        if (errno == EWOULDBLOCK) return {};
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        strerror_r(errno, buf, sizeof(buf));
        volume->debug->error("volume error: %s", buf);
        return {};
    }

    usize reopened_count = 0;
    FSFile** reopened = reopen_deleted_files(volume, arena, &reopened_count);

    usize event_count = 2ULL * (usize)kevent_count;
    events->count = 0;
    events->items = alloc_events(arena, event_count + system_count + reopened_count);
    if (!events->items)
        return {};
    for (usize i = 0; i < reopened_count; i++) {
        events->items[events->count++] = (FSEvent) {
            .file = reopened[i],
            .kind = FSEventKind_Create,
        };
    }

    usize deleted_just_now_count = 0;
    auto* deleted_just_now = (FSEvent**)arena->push(kevent_count * sizeof(FSEvent*), alignof(FSEvent));

    for (int i = 0; i < kevent_count; i++) {
        if (out_events[i].flags & EV_ERROR) {
            if (volume->debug) {
                char buf[1024];
                memset(buf, 0, sizeof(buf));
                strerror_r((int)out_events[i].data, buf, sizeof(buf));
                volume->debug->error("volume error: %s", buf);
            }
            continue;
        }
        FSFile* file = (FSFile*)out_events[i].udata;
        if (out_events[i].fflags & NOTE_DELETE) {
            VERIFY(file->kind == FSFileMount_SystemMount);
            file->system_mount.deleted_at = core_time_since_start();
            usize id = events->count++;
            events->items[id] = (FSEvent) {
                .file = file,
                .kind = FSEventKind_Delete,
            };
            if (deleted_just_now) deleted_just_now[deleted_just_now_count++] = &events->items[id];
            continue;
        }
        if (out_events[i].fflags & NOTE_WRITE) {
            VERIFY(file->kind == FSFileMount_SystemMount);
            file->system_mount.stale_at = core_time_since_start();
            usize id = events->count++;
            events->items[id] = (FSEvent) {
                .file = file,
                .kind = FSEventKind_Modify,
            };
            continue;
        }
    }

    reopened_count = 0;
    reopened = reopen_deleted_files(volume, arena, &reopened_count);
    if (!reopened) return *events;

    auto steal_delete_event = [&](usize reopened_index){
        if (!deleted_just_now) return false;
        for (usize i = 0; i < deleted_just_now_count; i++) {
            auto* event = deleted_just_now[i];
            if (event->file == reopened[reopened_index]) {
                if (event->kind == FSEventKind_Delete) {
                    event->kind = FSEventKind_Modify;
                    return true;
                }
            }
        }
        return false;
    };

    for (usize i = 0; i < reopened_count; i++) {
        if (steal_delete_event(i))
            continue;
        usize id = events->count++;
        events->items[id] = (FSEvent) {
            .file = reopened[i],
            .kind = FSEventKind_Modify,
        };
    }

    return *events;
}


C_API bool fs_system_open(Allocator* gpa, StringSlice path, FSFile* out)
{
    char* path_cstr = (char*)memclone_zero_extend(gpa, path.items, path.count, 1, 1);
    if (!path_cstr) return false;
    Defer free_path_cstr = [&]{
        memfree(gpa, path_cstr, path.count + 1, 1);
    };

    int fd = open(path_cstr, O_RDONLY);
    if (fd < 0) return false;

    struct stat st;
    if (fstat(fd, &st) < 0) return false;

    char* mapped_file = (char*)"";
    if (st.st_size != 0) {
        mapped_file = (char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped_file == MAP_FAILED) return false;
    }

    *out = (FSFile) {
        .gpa = gpa,
        .virtual_path = string_slice_empty(),
        .system_mount = (FSSystemMount) {
            .content = string_slice(mapped_file, st.st_size),
            .path = string_slice(path_cstr, path.count),
            .fd = fd,
            .stale_at = 0.0,
            .deleted_at = 0.0,
        },
        .kind = FSFileMount_SystemMount,
    };

    free_path_cstr.disarm();
    return true;
}

C_API FSFile fs_virtual_open(StringSlice path, StringSlice content)
{
    return (FSFile) {
        .gpa = nullptr,
        .virtual_path = path,
        .virtual_mount = (FSVirtualMount) {
            .content = content,
        },
        .kind = FSFileMount_SystemMount,
    };
}


C_API StringSlice fs_virtual_path(FSFile file)
{
    if (file.virtual_path.count) return file.virtual_path;
    switch (file.kind) {
    case FSFileMount_VirtualMount:
        return file.virtual_path;
    case FSFileMount_SystemMount:
        return file.system_mount.path;
    }
}


C_API bool fs_file_reload(FSFile* file)
{
    switch (file->kind) {
    case FSFileMount_VirtualMount:
        return true;
    case FSFileMount_SystemMount:
        break;
    }
    auto* mount = &file->system_mount;

    usize old_size = mount->content.count;
    void* old_ptr = (void*)mount->content.items;

    struct stat st;
    if (fstat(mount->fd, &st) < 0)
        return false;
    usize size = st.st_size;
    void* new_content = mmap(0, size, PROT_READ, MAP_PRIVATE, mount->fd, 0);
    if (new_content == MAP_FAILED)
        return false;
    mount->content.items = (char const*)new_content;
    mount->content.count = size;
    if (old_ptr != nullptr && old_size != 0) {
        munmap(old_ptr, old_size);
    }

    mount->stale_at = 0.0;
    return true;
}


C_API StringSlice fs_content(FSFile file)
{
    switch (file.kind) {
    case FSFileMount_SystemMount:
        return file.system_mount.content;
    case FSFileMount_VirtualMount:
        return file.virtual_mount.content;
    }
}

C_API bool fs_volume_needs_reload(FSVolume const* volume)
{
    for (usize i = 0; i < volume->count; i++) {
        if (fs_file_needs_reload(&volume->items[i]))
            return true;
    }
    return false;
}

C_API bool fs_volume_reload(FSVolume* volume)
{
    bool all_ok = true;
    for (usize i = 0; i < volume->count; i++) {
        FSFile* file = &volume->items[i];
        if (fs_file_needs_reload(file)) {
            all_ok &= fs_file_reload(file);
        }
    }
    return all_ok;
}

C_API bool fs_file_needs_reload(FSFile const* file)
{
    switch (file->kind) {
    case FSFileMount_VirtualMount:
        return false;
    case FSFileMount_SystemMount:
        return file->system_mount.stale_at != 0.0;
    }
}

static bool expand_if_needed(FSVolume* volume)
{
    if (volume->count >= volume->capacity) {
        usize new_capacity = volume->capacity == 0 ? 16 : volume->capacity * 2;
        auto* files = (FSFile*)memrealloc(page_allocator(), volume->items, volume->capacity * sizeof(FSFile), new_capacity * sizeof(FSFile), alignof(FSFile));
        if (!files) return false;
        volume->items = files;
        volume->capacity = new_capacity;
    }
    return true;
}
