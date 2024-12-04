#pragma once
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAR_PATH_MAX (100)
#define TAR_FILE_SIZE_MAX (077777777777)

typedef struct TarHeader {
    char path[TAR_PATH_MAX];
    char mode[8];           // octal
    char owner[8];          // octal
    char group[8];          // octal
    char file_size[12];     // octal
    char mtime[12];         // octal UNIX time
    char checksum[8];       // octal
    char link_indicator;    // 0 => normal file, 1 => hard link, 2 => symlink
    char link_path[TAR_PATH_MAX];
    char pad[255];
    u8 file_content[];
} TarHeader;
ty_static_assert(ty_offsetof(TarHeader, path) == 0);
ty_static_assert(ty_offsetof(TarHeader, mode) == 100);
ty_static_assert(ty_offsetof(TarHeader, owner) == 108);
ty_static_assert(ty_offsetof(TarHeader, group) == 116);
ty_static_assert(ty_offsetof(TarHeader, file_size) == 124);
ty_static_assert(ty_offsetof(TarHeader, mtime) == 136);
ty_static_assert(ty_offsetof(TarHeader, checksum) == 148);
ty_static_assert(ty_offsetof(TarHeader, link_indicator) == 156);
ty_static_assert(ty_offsetof(TarHeader, link_path) == 157);
ty_static_assert(sizeof(TarHeader) == 512);

typedef struct Tar Tar;

typedef struct TarFile {
    u8* bytes;
    usize size;
} TarFile;

Tar* tar_create(void); // FIXME: Add allocator
void tar_destroy(Tar* tar);
int tar_collect_garbage(Tar* tar);

isize tar_add(Tar* tar, c_string path, void const* content, usize content_size);
void tar_remove(Tar* tar, c_string path);
isize tar_find(Tar* tar, c_string path);
TarHeader* tar_header(Tar* tar, usize index);
TarFile tar_file(Tar* tar, usize index);
TarFile tar_as_file(Tar* tar);

#if __cplusplus
}
#endif
