#include "./Tar.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct Tar {
    TarFile arena;
    isize* headers;
    usize header_count;
} Tar;

#define NULL_FILE ((TarFile){ 0 })
#define NULL_HEADER ((TarHeader){ 0 })

static TarHeader* header_ptr(Tar* tar, isize index);

Tar* tar_create(void)
{
    Tar* tar = (Tar*)malloc(sizeof(Tar));
    if (!tar) {
        return NULL;
    }
    memset(tar, 0, sizeof(*tar));
    tar->arena = (TarFile){
        .bytes = calloc(1, sizeof(NULL_HEADER)),
        .size = sizeof(NULL_HEADER),
    };
    return tar;
}

void tar_destroy(Tar* tar)
{
    if (tar->arena.bytes) {
        free(tar->arena.bytes);
    }
    if (tar->headers) {
        free(tar->headers);
    }
}

int tar_collect_garbage(Tar* tar)
{
    if (tar->header_count == 0) return 0;

    usize new_header_count = tar->header_count;
    for (usize i = 0; i < tar->header_count; i++) {
        isize header = tar->headers[i];
        if (header < 0) continue;
        isize swap = tar->headers[new_header_count - 1];
        tar->headers[new_header_count - 1] = header;
        tar->headers[i] = swap;
        new_header_count -= 1;
    }
    if (tar->header_count == new_header_count) return 0;
    tar->header_count = new_header_count;

    u8* new_arena = (u8*)calloc(tar->arena.size, 1);
    if (!new_arena) return -1;
    usize new_arena_size = 0;
    for (usize i = 0; i < tar->header_count; i++) {
        unsigned size = 0;
        TarHeader* header = header_ptr(tar, tar->headers[i]);
        if (sscanf(header->file_size, "%011o", &size) < 0) {
            free(new_arena);
            return -1;
        }
        memcpy(new_arena + new_arena_size, header, sizeof(*header));
        new_arena_size += sizeof(TarHeader);
        memcpy(new_arena + new_arena_size, header->file_content, size);
        new_arena_size += size;
    }
    tar->arena = (TarFile) {
        .bytes = new_arena,
        .size = new_arena_size,
    };

    return 0;
}

isize tar_add(Tar* tar, c_string path, void const* content, usize content_size)
{
    usize path_len = strlen(path);
    if (path_len >= TAR_PATH_MAX) {
        return -1;
    }
    if (content_size > TAR_FILE_SIZE_MAX) {
        return -1;
    }

    usize header_index = tar->header_count;
    void* new_headers = realloc(tar->headers, (tar->header_count + 1) * sizeof(TarHeader*));
    if (!new_headers) {
        return -1;
    }
    tar->headers = (isize*)new_headers;
    tar->headers[header_index] = -1;
    tar->header_count += 1;

    usize new_size = tar->arena.size + sizeof(TarHeader) + content_size;
    void* new_bytes = realloc(tar->arena.bytes, new_size);
    if (!new_bytes) {
        return -1;
    }
    memset(new_bytes + tar->arena.size, 0, new_size - tar->arena.size);
    usize last_header_index = tar->arena.size - sizeof(NULL_HEADER);
    tar->arena = (TarFile) {
        .bytes = (u8*)new_bytes,
        .size = new_size,
    };
    tar->headers[header_index] = last_header_index;
    TarHeader* header = header_ptr(tar, last_header_index);
    memset(header, 0, sizeof(TarHeader));
    memcpy(header->path, path, path_len);

    snprintf(header->mode, sizeof(header->mode), "%060o ", 0666);

    snprintf(header->file_size, sizeof(header->file_size), "%010o ", (unsigned)content_size);
    memset(header->checksum, ' ', sizeof(header->checksum));
    unsigned checksum = sizeof(header->checksum) * ' ';
    for (usize i = 0; i < ty_offsetof(TarHeader, checksum); i++) {
        checksum += ((u8*)header)[i];
    }
    checksum %= 0777777;
    snprintf(header->checksum, sizeof(header->checksum), "%06o ", checksum);
    memcpy(header->file_content, content, content_size);

    return header_index;
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
    tar->headers[index] = -1;
}

isize tar_find(Tar* tar, c_string path)
{
    usize path_len = strlen(path);
    if (path_len >= TAR_PATH_MAX) {
        return -1;
    }
    if (!tar->headers) {
        return -1;
    }

    for (usize i = 0; i < tar->header_count; i++) {
        TarHeader* header = header_ptr(tar, tar->headers[i]);
        if (!header) continue;
        if (strcmp(path, header->path) == 0) {
            return i;
        }
    }

    return -1;
}

TarHeader* tar_header(Tar* tar, usize index)
{
    if (index >= tar->header_count) {
        return NULL;
    }
    return header_ptr(tar, tar->headers[index]);
}

TarFile tar_file(Tar* tar, usize index)
{
    if (index >= tar->header_count) {
        return NULL_FILE;
    }
    TarHeader* header = header_ptr(tar, tar->headers[index]);
    unsigned size = 0;
    int res = sscanf(header->file_size, "%011o", &size);
    if (res < 0) {
        return NULL_FILE;
    }
    return (TarFile){
        .bytes = header->file_content,
        .size = size,
    };
}

TarFile tar_as_file(Tar* tar)
{
    return tar->arena;
}

static TarHeader* header_ptr(Tar* tar, isize index)
{
    if (index < 0) {
        return NULL;
    }
    return (TarHeader*)tar->arena.bytes + index;
}
