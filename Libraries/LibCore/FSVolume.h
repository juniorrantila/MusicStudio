#pragma once
#include "./Forward.h"

#include <Basic/Bits.h>
#include <Basic/Logger.h>
#include <Basic/Allocator.h>
#include <Basic/StringSlice.h>
#include <Basic/FixedArena.h>

#include <sys/syslimits.h>
#include <sys/event.h>

#if __cplusplus
#include <LibTy/Optional.h>
#endif

typedef enum FSFileMount : u8 {
    FSFileMount_VirtualMount,
    FSFileMount_SystemMount,
} FSFileMount;

typedef struct FSVirtualMount {
    StringSlice content;
} FSVirtualMount;

typedef struct FSSystemMount {
    StringSlice content;
    StringSlice path;
    int fd;
    f64 stale_at;
    f64 deleted_at;
} FSSystemMount;

typedef struct FSFile {
    Allocator* gpa;
    StringSlice virtual_path;
    union {
        FSVirtualMount virtual_mount;
        FSSystemMount system_mount;
    };
    FSFileMount kind;
} FSFile;
static_assert(sizeof(FSFile) == 88);

typedef struct { u16 index; } FileID; static_assert(ty_bits_fitting(OPEN_MAX) < 16);

typedef enum FSEventKind : u8 {
    FSEventKind_Modify,
    FSEventKind_Delete,
    FSEventKind_Create,
} FSEventKind;

constexpr u64 fs_open_directory_max = 256;
constexpr u64 fs_open_file_max = OPEN_MAX - fs_open_directory_max;
constexpr u64 fs_open_max = OPEN_MAX;

typedef struct FSEventID { u16 index; } FSEventID;

typedef struct FSEvents {
    FileID file[fs_open_max * 3]; // Create, Write, Delete
    FSEventKind kind[fs_open_max * 3]; // Create, Write, Delete
    u16 count; static_assert(ty_bits_fitting(fs_open_max) < 16);
} FSEvents;

typedef struct FSVolume {
    u16 count; static_assert(ty_bits_fitting(fs_open_file_max) < 16);
    u16 unique_directory_count; static_assert(ty_bits_fitting(fs_open_directory_max) < 16);

    int watch_fd;
    bool automount_when_not_found;

    FSFile items[fs_open_file_max];

    struct {
        struct kevent in_events[fs_open_max];
        struct kevent out_events[fs_open_max];
        FileID reopened_files[fs_open_max];
        FSEventID deleted_just_now[fs_open_max];
        FSEvents events;
    } trans;

#ifdef __cplusplus
    Optional<StringSlice const*> open(StringSlice path);
    FSFile use(FileID);
    FSFile* use_ref(FileID);
    Optional<FileID> find(StringSlice path);

    Optional<FileID> mount(FSFile file);
#endif
} FSVolume;
static_assert(sizeof(FSVolume) < 2 * MiB);

C_API void fs_volume_init(FSVolume*);

C_API StringSlice const* fs_volume_open(FSVolume*, StringSlice path);
C_API FSFile fs_volume_use(FSVolume*, FileID);
C_API FSFile* fs_volume_use_ref(FSVolume*, FileID);
C_API bool fs_volume_find(FSVolume*, StringSlice path, FileID*);

C_API [[nodiscard]] bool fs_volume_mount(FSVolume*, FSFile file, FileID*);

struct timespec;
C_API FSEvents fs_volume_poll_events(FSVolume*, struct timespec const* timeout);
C_API bool fs_volume_needs_reload(FSVolume const*);
C_API bool fs_volume_reload(FSVolume*);

C_API bool fs_system_open(Allocator* gpa, StringSlice path, FSFile*);
C_API bool fs_file_needs_reload(FSFile const*);
C_API bool fs_file_reload(FSFile*);
C_API StringSlice fs_content(FSFile);

C_API StringSlice fs_virtual_path(FSFile);

C_API FSFile fs_virtual_open(StringSlice path, StringSlice content);
