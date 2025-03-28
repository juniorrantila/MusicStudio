#include "./FSVolume.h"
#include "Ty/StringSlice.h"

#include <Ty/Verify.h>
#include <Ty/Defer.h>
#include <Ty/Allocator.h>
#include <Ty2/PageAllocator.h>

#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

static bool expand_if_needed(FSVolume* volume);


Optional<FSVolume*> FSVolume::create(Allocator* gpa) { return fs_volume_create(gpa); }
C_API FSVolume* fs_volume_create(Allocator* gpa)
{
    FSVolume* volume = (FSVolume*)memalloc(gpa, sizeof(FSVolume), alignof(FSVolume));
    if (!volume) return nullptr;
    *volume = (FSVolume){};
    memset(volume, 0, sizeof(FSVolume));
    volume->gpa = gpa;
    volume->watch_fd = -1;
    return volume;
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


Optional<StringSlice const*> FSVolume::open(StringSlice path) const { return fs_volume_open(this, path); }
C_API StringSlice const* fs_volume_open(FSVolume const* volume, StringSlice path)
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

Optional<FileID> FSVolume::find(StringSlice path) const
{
    FileID id;
    if (!fs_volume_find(this, path, &id))
        return {};
    return id;
}
C_API bool fs_volume_find(FSVolume const* volume, StringSlice path, FileID* id)
{
    auto resolved_path = path.resolve_path(volume->gpa);
    if (!resolved_path.has_value()) {
        return false;
    }
    defer [&] {
        volume->gpa->free((char*)resolved_path->items, resolved_path->count);
    };
    for (usize i = 0; i < volume->count; i++) {
        auto other = fs_virtual_path(volume->items[i]);
        if (resolved_path->equal(other)) {
            if (id) *id = (FileID){ .index = i };
            return true;
        }
    }

    if (volume->automount_when_not_found) {
        auto* log = volume->debug;
        if (log) log->debug("could not find '%.*s', trying to auto mount it", (int)path.count, path.items);
        FSFile file;
        if (!fs_system_open(volume->gpa, path, &file)) {
            if (log) log->debug("could not open '%.*s'", (int)path.count, path.items);
            return false;
        }
        if (!mount_no_cache((FSVolume*)volume, file, id)) {
            if (log) log->debug("could not mount '%.*s'", (int)path.count, path.items);
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


C_API FSEvents fs_volume_poll_events(FSVolume* volume, struct timespec* timeout)
{
    auto* arena = &volume->event_arena;
    if (volume->watch_fd < 0) {
        int fd = kqueue();
        if (fd < 0) return {};
        volume->watch_fd = fd;
        volume->event_arena = arena_create(volume->gpa);
        arena = &volume->event_arena;
    }
    arena->drain();
    FSEvents* events = &volume->events;
    memset(events, 0, sizeof(*events));

    usize file_count = volume->count;
    struct kevent* in_events = (struct kevent*)arena->alloc(file_count * sizeof(*in_events), alignof(struct kevent));
    if (!in_events) return {};
    memset(in_events, 0, file_count * sizeof(*in_events));

    struct kevent* out_events = (struct kevent*)arena->alloc(file_count * sizeof(*out_events), alignof(struct kevent));
    if (!out_events) return {};
    memset(out_events, 0, file_count * sizeof(*out_events));

    int event_count = 0;
    for (usize i = 0; i < file_count; i++) {
        FSFile* file = &volume->items[i];
        if (file->kind == FSFileMount_SystemMount) {
            int id = event_count++;
            in_events[id] = (struct kevent) {
                .ident = (uptr)file->system_mount.fd,
                .filter = EVFILT_VNODE,
                .flags = EV_ADD | EV_ONESHOT,
                .fflags = NOTE_WRITE | NOTE_DELETE,
                .data = 0,
                .udata = file,
            };
        }
    }
    event_count = kevent(volume->watch_fd, in_events, event_count, out_events, event_count, timeout);
    if (event_count < 0) return {};

    events->items = (FSEvent*)arena->alloc(
        event_count * sizeof(*events->items),
        alignof(FSEvent)
    );
    if (!events->items)
        return {};
    memset(events->items, 0, event_count * sizeof(*events->items));

    for (int i = 0; i < event_count; i++) {
        if (out_events[i].flags & EV_ERROR) {
            continue;
        }
        FSFile* file = (FSFile*)out_events[i].udata;
        if (out_events[i].fflags & NOTE_DELETE) {
            usize id = events->count++;
            events->items[id] = (FSEvent) {
                .file = file,
                .kind = FSEventKind_Delete,
            };
            continue;
        }
        if (out_events[i].fflags & NOTE_WRITE) {
            VERIFY(file->kind == FSFileMount_SystemMount);
            file->system_mount.stale = true;
            usize id = events->count++;
            events->items[id] = (FSEvent) {
                .file = file,
                .kind = FSEventKind_Modify,
            };
            continue;
        }
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
            .stale = false,
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

    mount->stale = false;
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
        break;
    }
    FSSystemMount const* mount = &file->system_mount;
    return mount->stale;
}

static bool expand_if_needed(FSVolume* volume)
{
    if (volume->count >= volume->capacity) {
        usize new_capacity = volume->capacity == 0 ? 16 : volume->capacity * 2;
        auto* files = (FSFile*)memrealloc(volume->gpa, volume->items, volume->capacity * sizeof(FSFile), new_capacity * sizeof(FSFile), alignof(FSFile));
        if (!files) return false;
        volume->items = files;
        volume->capacity = new_capacity;
    }
    return true;
}
