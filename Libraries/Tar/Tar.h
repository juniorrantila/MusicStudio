#pragma once
#include <Ty/Base.h>
#include <Ty/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tar Tar;

typedef struct TarFile {
    u8 const* bytes;
    usize size;
} TarFile;

typedef enum e_tar : isize {
    e_tar_none = 0,
    e_tar_no_mem,
    e_tar_no_ent,
    e_tar_invalid_path,
    e_tar_invalid_file_size,
    e_tar_invalid_buffer_size,
} e_tar;

c_string tar_strerror(e_tar error);

Tar* tar_create(Allocator* gpa);
void tar_destroy(Tar* tar);

isize tar_add(Tar* tar, c_string path, void const* content, usize content_size);
isize tar_add_borrowed(Tar* tar, c_string path, void const* content, usize content_size);
void tar_remove(Tar* tar, c_string path);
void tar_remove_at_index(Tar* tar, usize index);

usize tar_file_count(Tar const* tar);
c_string tar_file_path(Tar const* tar, usize index);

isize tar_find(Tar const* tar, c_string);
TarFile tar_file(Tar const* tar, c_string);
TarFile tar_file_at_index(Tar const* tar, usize);

isize tar_buffer(Tar const* tar, u8* buffer, usize buffer_size);
usize tar_buffer_size(Tar const* tar);

#if __cplusplus
}
#endif

#ifdef __cplusplus
static inline c_string to_c_string(e_tar error) { return tar_strerror(error); }
#endif
