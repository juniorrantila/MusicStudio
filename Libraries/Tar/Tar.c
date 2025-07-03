#include "./Tar.h"

#include <Ty2/Allocator.h>
#include <Ty2/Verify.h>

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

static constexpr u64 tar_path_max = 100;
static constexpr u64 tar_file_size_max = 077777777777;

typedef struct TarHeader {
    char path[tar_path_max];
    char mode[8];           // octal
    char owner[8];          // octal
    char group[8];          // octal
    char file_size[12];     // octal
    char mtime[12];         // octal UNIX time
    char checksum[8];       // octal
    char link_indicator;    // 0 => normal file, 1 => hard link, 2 => symlink
    char link_path[tar_path_max];
    char pad[255];
} TarHeader;
static_assert(ty_offsetof(TarHeader, path) == 0);
static_assert(ty_offsetof(TarHeader, mode) == 100);
static_assert(ty_offsetof(TarHeader, owner) == 108);
static_assert(ty_offsetof(TarHeader, group) == 116);
static_assert(ty_offsetof(TarHeader, file_size) == 124);
static_assert(ty_offsetof(TarHeader, mtime) == 136);
static_assert(ty_offsetof(TarHeader, checksum) == 148);
static_assert(ty_offsetof(TarHeader, link_indicator) == 156);
static_assert(ty_offsetof(TarHeader, link_path) == 157);
static_assert(sizeof(TarHeader) == 512);
static constexpr TarHeader tar_header_null = (TarHeader){0};

typedef struct {
    char path[tar_path_max];
    char file_size[12];     // octal

} DecodedHeader;

static bool decode_header(TarHeader const*, char const** path, u64* path_size, u8 const** content, u64* content_size);
static u64 decode_octal(char const* buf, u64 size);

C_API Tar tar_init(void const* data, u64 size)
{
    return (Tar){
        .base = data,
        .head = data,
        .seek = data,
        .end = data + size,
    };
}

C_API bool untar(Tar* tar, char const** path, u64* path_size, u8 const** content, u64* content_size)
{
    while (tar->seek < tar->end) {
        TarHeader* header = (TarHeader*)tar->seek;
        tar->seek += sizeof(TarHeader);

        if (!decode_header(header, path, path_size, content, content_size))
            continue;

        if (*content + *content_size > tar->end)
            continue;

        return true;
    }
    return false;
}

C_API void tar_rewind(Tar* tar)
{
    tar->seek = tar->base;
}

C_API TarCounter tar_counter(void)
{
    return (TarCounter){sizeof(TarHeader)};
}

C_API void tar_count(TarCounter* tar, u64 content_size)
{
    tar->size += sizeof(TarHeader) + content_size;
    tar->size = (u64)__builtin_align_up((void*)tar->size, 512);
}

static bool tar_alloc(Tar*, u64 size, TarHeader**, u8** content);

static void header_init(char const* path, usize path_size, usize content_size, TarHeader* header)
{
    memset(header->path, ' ', sizeof(header->path));
    memcpy(header->path, path, path_size);

    snprintf(header->mode, sizeof(header->mode), "%06o ", 0666);

    snprintf(header->file_size, sizeof(header->file_size), "%010o ", (unsigned)content_size);
    memset(header->checksum, ' ', sizeof(header->checksum));
    unsigned checksum = sizeof(header->checksum) * ' ';
    for (usize i = 0; i < ty_offsetof(TarHeader, checksum); i++) {
        checksum += ((u8*)header)[i];
    }
    checksum %= 0777777;
    snprintf(header->checksum, sizeof(header->checksum), "%06o ", checksum);
}

C_API e_tar tar_add(Tar* tar, c_string path, void const* content, usize content_size)
{
    return tar_add2(tar, path, strlen(path), content, content_size);
}

C_API e_tar tar_add2(Tar* tar, char const* path, usize path_size, void const* content, usize content_size)
{
    if (path_size >= tar_path_max)
        return e_tar_invalid_path;
    if (content_size > tar_file_size_max)
        return e_tar_invalid_file_size;

    TarHeader* header;
    u8* tar_content;
    if (!tar_alloc(tar, content_size, &header, &tar_content))
        return -e_tar_no_mem;
    header_init(path, path_size, content_size, header);
    memcpy(tar_content, content, content_size);
    return e_tar_none;
}

C_API bool tar_find(Tar const* tar, char const* path, u8 const** content, u64* content_size)
{
    return tar_find2(tar, path, strlen(path), content, content_size);
}

C_API bool tar_find2(Tar const* tar, char const* path, u64 path_size, u8 const** content, u64* content_size)
{
    if (path_size >= tar_path_max)
        return false;
    char q[tar_path_max];
    memset(q, ' ', path_size);
    memcpy(q, path, path_size);
    for (
        TarHeader const* entry = (TarHeader const*)tar->base;
        entry < (TarHeader const*)tar->end;
        entry += 1
    ) {
        char const* entry_path;
        u64 entry_path_size;
        if (!decode_header(entry, &entry_path, &entry_path_size, content, content_size))
            continue;
        if (memcmp(entry_path, q, entry_path_size) == 0)
            return true;
    }
    return false;
}

static bool tar_alloc(Tar* tar, u64 size, TarHeader** header, u8** content)
{
    TarHeader* head = (TarHeader*)tar->head;
    u8* data = (u8*)(head + 1);
    u8* end = __builtin_align_up(data + size, 512);
    if (end + sizeof(TarHeader) > tar->end)
        return false;
    *((TarHeader*)end) = tar_header_null;
    tar->head = end;
    *header = head;
    *content = (void*)data;
    return true;
}

static bool decode_header(TarHeader const* header, char const** path, u64* path_size, u8 const** content, u64* content_size)
{
    TarHeader h = *header;
    u64 checksum = decode_octal(h.checksum, sizeof(h.checksum));
    memset(h.checksum, ' ', sizeof(h.checksum));

    i8* ibuf = (i8*)header;
    u8* ubuf = (u8*)header;
    u64 usum = 0;
    u64 isum = 0;
    for (u32 i = 0; i < sizeof(TarHeader); i++) {
        isum += ibuf[i];
        usum += ubuf[i];
    }
    if (usum != checksum && isum != checksum) {
        return false;
    }

    *content = (void const*)&header[1];
    *content_size = decode_octal(h.file_size, sizeof(h.file_size));

    char* path_end = &h.path[sizeof(h.path) - 1];
    while (path_end > h.path) {
        char e = *path_end;
        if (e != ' ' && e != '\0') break;
        path_end -= 1;
    }
    *path_size = path_end - h.path;
    *path = h.path;

    return true;
}

static u64 decode_octal(char const* buf, u64 size)
{
    char const* end = buf + size;

    while (end > buf) {
        char e = *end;
        if (e != ' ' && e != '\0') break;
        end -= 1;
    }

    u64 radix = 1;
    u64 result = 0;
    for (char const* c = buf; c < end; c += 1) {
        u8 n = ((u8)*c) - (u8)'0';
        result += n * radix;
        radix *= 8;
    }
    return result;
}

c_string tar_strerror(e_tar error)
{
    if (((isize)error) < 0) {
        error = (e_tar)-(isize)error;
    }
    switch (error) {
    case e_tar_none:
        return "no error";
    case e_tar_invalid_file:
        return "file is not a tar";
    case e_tar_invalid_path:
        return "invalid path";
    case e_tar_invalid_file_size:
        return "invalid path size";
    case e_tar_invalid_buffer_size:
        return "invalid buffer size";
    case e_tar_no_mem:
        return "out of memory";
    }
    return "unknown";
}
