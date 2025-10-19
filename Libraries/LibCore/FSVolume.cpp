#include "./FSVolume.h"

#include <Basic/StringSlice.h>
#include <Basic/Base.h>
#include <Basic/FixedArena.h>
#include <Basic/Verify.h>
#include <Basic/Defer.h>
#include <Basic/PageAllocator.h>
#include <Basic/Context.h>

#include "./Time.h"

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

C_API void fs_volume_init(FSVolume* volume)
{
    *volume = (FSVolume){};
    memset(volume, 0, sizeof(FSVolume));
    volume->watch_fd = -1;
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


FSFile FSVolume::use(FileID id) { return fs_volume_use(this, id); }
C_API FSFile fs_volume_use(FSVolume* volume, FileID id)
{
    return *fs_volume_use_ref(volume, id);
}

FSFile* FSVolume::use_ref(FileID id) { return fs_volume_use_ref(this, id); }
C_API FSFile* fs_volume_use_ref(FSVolume* volume, FileID id)
{
    VERIFY(id.index < volume->count);
    return &volume->items[id.index];
}

static bool mount_no_cache(FSVolume* volume, FSFile file, FileID* out)
{
    if (verify(volume->count < ARRAY_SIZE(volume->items)).failed)
        return false;
    u16 id = volume->count++;
    volume->items[id] = file;
    if (out) *out = (FileID){ id };
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
    StringSlice resolved_path = sv_empty();
    if (!sv_resolve_path(temporary_arena(), path, "."s, &resolved_path))
        return false;

    for (u16 i = 0; i < volume->count; i++) {
        auto other = fs_virtual_path(volume->items[i]);
        if (sv_equal(resolved_path, other)) {
            if (id) *id = (FileID){ .index = i };
            return true;
        }
    }

    if (volume->automount_when_not_found) {
        warnf("could not find '%.*s', trying to auto mount it", (int)path.count, path.items);
        FSFile file;
        if (!fs_system_open(page_allocator(), path, &file)) {
            warnf("could not open '%.*s'", (int)path.count, path.items);
            return false;
        }
        if (!mount_no_cache(volume, file, id)) {
            warnf("could not mount '%.*s'", (int)path.count, path.items);
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

static int fill_in_events(FSVolume* volume)
{
    int event_id = 0;
    for (u64 i = 0; i < volume->count; i++) {
        FSFile* file = &volume->items[i];
        if (file->kind == FSFileMount_SystemMount) {
            u64 id = event_id++;
            VERIFY(id < ARRAY_SIZE(volume->trans.in_events));
            volume->trans.in_events[id] = (struct kevent) {
                .ident = (uptr)file->system_mount.fd,
                .filter = EVFILT_VNODE,
                .flags = EV_ADD | EV_ONESHOT,
                .fflags = NOTE_WRITE | NOTE_DELETE,
                .data = 0,
                .udata = (void*)(uptr)i,
            };
        }
    }
    return event_id;
}

static FileID* reopen_deleted_files(FSVolume* volume, u64* count)
{
    u64 opened_count = 0;
    for (u16 i = 0; i < volume->count; i++) {
        auto* file = &volume->items[i];
        if (file->kind != FSFileMount_SystemMount)
            continue;
        if (!is_file_deleted(file))
            continue;
        auto* mount = &file->system_mount;
        int fd = open(mount->path.items, O_RDONLY);
        if (fd < 0) continue;
        mount->fd = fd;
        mount->stale_at = mount->deleted_at;
        mount->deleted_at = 0.0;
        u64 id = opened_count + 1;
        VERIFY(id < ARRAY_SIZE(volume->trans.reopened_files));
        volume->trans.reopened_files[id] = (FileID){i};
        opened_count = id;
    }

    return *count = opened_count, volume->trans.reopened_files;
}

C_API FSEvents fs_volume_poll_events(FSVolume* volume, struct timespec const* timeout)
{
    if (volume->watch_fd < 0) {
        int fd = kqueue();
        if (fd < 0) return {};
        volume->watch_fd = fd;
    }

    int in_event_count = fill_in_events(volume);
    int kevent_count = kevent(volume->watch_fd, volume->trans.in_events, in_event_count, volume->trans.out_events, in_event_count, timeout);
    if (kevent_count < 0) {
        if (errno == EWOULDBLOCK) return {};
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        strerror_r(errno, buf, sizeof(buf));
        errorf("could not poll events: %s", buf);
        return {};
    }

    FSEvents* events = &volume->trans.events;
    {
        events->count = 0;

        u64 reopened_count = 0;
        FileID* reopened = reopen_deleted_files(volume, &reopened_count);

        for (u64 i = 0; i < reopened_count; i++) {
            if (verify(events->count + 1 < ARRAY_SIZE(events->file)).failed)
                break;
            u64 id = events->count++;
            events->file[id] = reopened[i];
            events->kind[id] = FSEventKind_Create;
        }
    }

    u16 deleted_just_now_count = 0;
    for (int i = 0; i < kevent_count; i++) {
        if (volume->trans.out_events[i].flags & EV_ERROR) {
            char buf[1024];
            memset(buf, 0, sizeof(buf));
            strerror_r((int)volume->trans.out_events[i].data, buf, sizeof(buf));
            errorf("volume error: %s", buf);
            continue;
        }
        FileID file_id = (FileID){(u16)(uptr)volume->trans.out_events[i].udata};
        auto* file = fs_volume_use_ref(volume, file_id);
        if (volume->trans.out_events[i].fflags & NOTE_DELETE) {
            VERIFY(file->kind == FSFileMount_SystemMount);
            file->system_mount.deleted_at = core_time_since_unspecified_epoch();
            u16 id = events->count++;
            VERIFY(id < ARRAY_SIZE(events->file));
            events->file[id] = file_id;
            events->kind[id] = FSEventKind_Delete;
            if (verify(deleted_just_now_count + 1 < ARRAY_SIZE(volume->trans.deleted_just_now)).failed)
                continue;
            volume->trans.deleted_just_now[deleted_just_now_count++] = (FSEventID){id};
            continue;
        }
        if (volume->trans.out_events[i].fflags & NOTE_WRITE) {
            VERIFY(file->kind == FSFileMount_SystemMount);
            file->system_mount.stale_at = core_time_since_unspecified_epoch();
            u64 id = events->count++;
            VERIFY(id < ARRAY_SIZE(events->file));
            events->file[id] = file_id;
            events->kind[id] = FSEventKind_Modify;
            continue;
        }
    }

    {
        u64 reopened_count = 0;
        FileID* reopened = reopen_deleted_files(volume, &reopened_count);
        auto* events = &volume->trans.events;
        for (u64 reopened_index = 0; reopened_index < reopened_count; reopened_index++) {
            for (u64 i = 0; i < deleted_just_now_count; i++) {
                auto event_id = volume->trans.deleted_just_now[i];
                if (events->kind[event_id.index] == FSEventKind_Delete) {
                    if (events->file[event_id.index].index == reopened[reopened_index].index) {
                        events->kind[event_id.index] = FSEventKind_Modify;
                        goto next;
                    }
                }
            }
next:
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

    debugf("opened '%s'", path_cstr);

    *out = (FSFile) {
        .gpa = gpa,
        .virtual_path = sv_empty(),
        .system_mount = (FSSystemMount) {
            .content = sv_from_parts(mapped_file, st.st_size),
            .path = sv_from_parts(path_cstr, path.count),
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

    auto path = fs_virtual_path(*file);
    debugf("reloading %.*s", (u32)path.count, path.items);
    VERIFY(fs_file_needs_reload(file));

    u64 old_size = mount->content.count;
    void* old_ptr = (void*)mount->content.items;

    close(mount->fd);
    mount->fd = open(path.items, O_RDONLY);
    if (mount->fd < 0) {
        errorf("could not open '%.*s': %s", (u32)path.count, path.items, strerror(errno));
        return false;
    }

    struct stat st;
    if (fstat(mount->fd, &st) < 0) {
        errorf("could not stat '%.*s': %s", (u32)path.count, path.items, strerror(errno));
        return false;
    }
    
    if (st.st_size == 0) {
        mount->content = sv_empty();
        mount->deleted_at = 0.0;
        mount->stale_at = 0.0;
        return true;
    }

    u64 size = st.st_size;
    void* new_content = mmap(0, size, PROT_READ, MAP_PRIVATE, mount->fd, 0);
    if (new_content == MAP_FAILED) {
        errorf("could not map '%.*s': %s", (u32)path.count, path.items, strerror(errno));
        return false;
    }
    mount->content.items = (char const*)new_content;
    mount->content.count = size;
    if (old_ptr != nullptr && old_size != 0) {
        munmap(old_ptr, old_size);
    }

    mount->deleted_at = 0.0;
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
    for (u64 i = 0; i < volume->count; i++) {
        if (fs_file_needs_reload(&volume->items[i]))
            return true;
    }
    return false;
}

C_API bool fs_volume_reload(FSVolume* volume)
{
    bool all_ok = true;
    for (u64 i = 0; i < volume->count; i++) {
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
