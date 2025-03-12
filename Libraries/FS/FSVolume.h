#pragma once
#include "./Forward.h"

#include <Ty2/Allocator.h>
#include <Ty/StringSlice.h>
#include <Tar/Tar.h>
#include <Ty2/Arena.h>

#if __cplusplus
#include <Ty/Optional.h>
#endif

typedef enum FSFileMount {
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

typedef struct { usize index; } FileID;

typedef enum FSEventKind {
    FSEventKind_Modify,
    FSEventKind_Delete,
    // FSEventKind_Create,
} FSEventKind;

typedef struct FSEvent {
    FSFile* file;
    FSEventKind kind;
} FSEvent;

typedef struct FSEvents {
    FSEvent* items;
    usize count;
} FSEvents;

typedef struct FSVolume {
    Allocator* gpa;
    Arena event_arena;

    FSFile* items;
    usize count;
    usize capacity;

    FSEvents events;
    int watch_fd;

#ifdef __cplusplus
    static Optional<FSVolume*> create(Allocator* gpa);

    Optional<Tar*> as_tar(Allocator*) const;
    Optional<StringSlice*> open(StringSlice path) const;

    Optional<FileID> mount(FSFile file);
#endif
} FSVolume;

C_API FSVolume* fs_volume_create(Allocator* gpa);

C_API Tar* fs_volume_as_tar(FSVolume const*, Allocator*);
C_API StringSlice* fs_volume_open(FSVolume const*, StringSlice path);

C_API [[nodiscard]] bool fs_volume_mount(FSVolume*, FSFile file, FileID*);

C_API FSEvents fs_volume_poll_events(FSVolume*, struct timespec* timeout);
C_API bool fs_reload_needed(FSEvents);

C_API bool fs_system_open(Allocator* gpa, StringSlice path, FSFile*);
C_API bool fs_file_reload(FSFile*);
C_API StringSlice fs_content(FSFile);

C_API StringSlice fs_virtual_path(FSFile);

C_API FSFile fs_virtual_open(StringSlice path, StringSlice content);
