#include "./Tar.h"

#include <Ty/Allocator.h>

#include <string.h>
#include <stdio.h>
#include <stdint.h>


#define TAR_PATH_MAX (100)
#define TAR_FILE_SIZE_MAX (077777777777)
#define NULL_FILE ((TarFile){ 0 })
#define NULL_HEADER ((TarHeader){ 0 })


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


typedef struct TarEntry {
    TarHeader header;
    TarFile file;
    bool owns_file;
} TarEntry;


typedef struct Tar {
    Allocator* gpa;

    TarEntry* items;

    usize count;
    usize capacity;
} Tar;


Tar* tar_create(Allocator* gpa)
{
    usize capacity = 16;
    TarEntry* items = 0;

    Tar* tar = ty_alloc(gpa, sizeof(Tar), alignof(Tar));
    if (!tar) {
        return 0;
    }
    memset(tar, 0, sizeof(*tar));

    items = ty_alloc(gpa, capacity * sizeof(TarEntry), alignof(TarEntry));
    if (!items) {
        ty_free(gpa, tar, sizeof(Tar));
        return 0;
    }

    *tar = (Tar) {
        .gpa = gpa,
        .items = items,
        .capacity = capacity,
    };
    return tar;
}


void tar_destroy(Tar* tar)
{
    for (usize i = 0; i < tar->count; i++) {
        TarEntry* entry = &tar->items[i];
        if (entry->owns_file) {
            ty_free(tar->gpa, (void*)entry->file.bytes, entry->file.size);
        }
    }
    if (tar->items) {
        ty_free(tar->gpa, tar->items, sizeof(TarEntry) * tar->capacity);
    }
    ty_free(tar->gpa, tar, sizeof(Tar));
}


static e_tar header_init(c_string path, usize content_size, TarHeader* header)
{
    usize path_len = strlen(path);
    if (path_len >= TAR_PATH_MAX) {
        return e_tar_invalid_path;
    }
    if (content_size > TAR_FILE_SIZE_MAX) {
        return e_tar_invalid_file_size;
    }

    memcpy(header->path, path, path_len);

    snprintf(header->mode, sizeof(header->mode), "%06o ", 0666);

    snprintf(header->file_size, sizeof(header->file_size), "%010o ", (unsigned)content_size);
    memset(header->checksum, ' ', sizeof(header->checksum));
    unsigned checksum = sizeof(header->checksum) * ' ';
    for (usize i = 0; i < ty_offsetof(TarHeader, checksum); i++) {
        checksum += ((u8*)header)[i];
    }
    checksum %= 0777777;
    snprintf(header->checksum, sizeof(header->checksum), "%06o ", checksum);
    return e_tar_none;
}


static e_tar expand_if_needed(Tar* tar)
{
    if (tar->count < tar->capacity) {
        return e_tar_none;
    }
    usize new_capacity = tar->capacity * 2;
    TarEntry* entries = ty_alloc(tar->gpa, new_capacity * sizeof(TarEntry), alignof(TarEntry));
    if (!entries) {
        return e_tar_no_mem;
    }
    memcpy(entries, tar->items, tar->capacity * sizeof(TarEntry));
    ty_free(tar->gpa, tar->items, tar->capacity * sizeof(TarEntry));
    tar->items = entries;
    tar->capacity = new_capacity;

    return e_tar_none;
}


isize tar_add(Tar* tar, c_string path, void const* content, usize content_size)
{
    e_tar error;
    TarHeader header = { 0 };
    error = header_init(path, content_size, &header);
    if (error != e_tar_none) {
        return -error;
    }

    if (tar->count + 1 >= SIZE_MAX / 2) {
        return -e_tar_no_mem;
    }

    u8* copy = ty_alloc(tar->gpa, content_size, 16);
    if (!copy) {
        return -e_tar_no_mem;
    }
    memcpy(copy, content, content_size);

    error = expand_if_needed(tar);
    if (error != e_tar_none) {
        ty_free(tar->gpa, copy, content_size);
        return -error;
    }
    usize index = tar->count++;
    tar->items[index] = (TarEntry) {
        .header = header,
        .file = (TarFile) {
            .bytes = copy,
            .size = content_size,
        },
        .owns_file = true,
    };
    return (isize)index;
}


isize tar_add_borrowed(Tar* tar, c_string path, void const* content, usize content_size)
{
    e_tar error;
    TarHeader header = { 0 };
    error = header_init(path, content_size, &header);
    if (error != e_tar_none) {
        return -error;
    }

    if (tar->count + 1 >= SIZE_MAX / 2) {
        return -e_tar_no_mem;
    }

    error = expand_if_needed(tar);
    if (error != e_tar_none) {
        return -error;
    }
    usize index = tar->count++;
    tar->items[index] = (TarEntry) {
        .header = header,
        .file = (TarFile) {
            .bytes = (u8 const*)content,
            .size = content_size,
        },
        .owns_file = true,
    };
    return (isize)index;
}


void tar_remove_at_index(Tar* tar, usize index)
{
    if (index >= tar->count)
        return;
    if (tar->count > 1) {
        tar->items[index] = tar->items[tar->count - 1];
    }
    tar->count -= 1;
}


void tar_remove(Tar* tar, c_string path)
{
    usize path_len = strlen(path);
    if (path_len >= TAR_PATH_MAX) {
        return;
    }

    isize index = tar_find(tar, path);
    if (index < 0) {
        return;
    }
    tar_remove_at_index(tar, index);
}


usize tar_file_count(Tar const* tar)
{
    return tar->count;
}


c_string tar_file_path(Tar const* tar, usize index)
{
    if (index >= tar->count) {
        return 0;
    }
    return tar->items[index].header.path;
}


isize tar_find(Tar const* tar, c_string path)
{
    usize path_len = strlen(path);
    if (path_len >= TAR_PATH_MAX) {
        return -e_tar_invalid_path;
    }
    if (!tar->items) {
        return -e_tar_no_ent;
    }

    for (usize i = 0; i < tar->count; i++) {
        TarHeader* header = &tar->items[i].header;
        if (strcmp(path, header->path) == 0) {
            return (isize)i;
        }
    }

    return -e_tar_no_ent;
}


TarFile tar_file_at_index(Tar const* tar, usize index)
{
    if (index >= tar->count) {
        return NULL_FILE;
    }
    return tar->items[index].file;
}


TarFile tar_file(Tar const* tar, c_string path)
{
    isize index = tar_find(tar, path);
    if (index < 0) {
        return NULL_FILE;
    }
    return tar_file_at_index(tar, index);
}


usize tar_buffer_size(Tar const* tar)
{
    usize size = sizeof(TarHeader);
    for (usize i = 0; i < tar->count; i++) {
        size += sizeof(TarHeader);
        size += tar->items[i].file.size;
        size += (512 - (size % 512));
    }
    return size;
}


isize tar_buffer(Tar const* tar, u8* buffer, usize buffer_size)
{
    usize size = tar_buffer_size(tar);
    if (size > buffer_size) {
        return -e_tar_invalid_buffer_size;
    }
    memset(buffer, 0, buffer_size);
    usize cursor = 0;
    for (usize i = 0; i < tar->count; i++) {
        TarEntry* entry = &tar->items[i];
        memcpy(&buffer[cursor], &entry->header, sizeof(entry->header));
        cursor += sizeof(TarHeader);

        memcpy(&buffer[cursor], entry->file.bytes, entry->file.size);
        cursor += (512 - (cursor % 512));
    }

    return (isize)size;
}


c_string tar_strerror(e_tar error)
{
    if (((isize)error) < 0) {
        error = (e_tar)-(isize)error;
    }
    switch (error) {
    case e_tar_none:
        return "no error";
    case e_tar_no_mem:
        return "out of memory";
    case e_tar_no_ent:
        return "no entry found";
    case e_tar_invalid_path:
        return "invalid path";
    case e_tar_invalid_file_size:
        return "invalid path size";
    case e_tar_invalid_buffer_size:
        return "invalid buffer size";
    }
    return "unknown";
}
