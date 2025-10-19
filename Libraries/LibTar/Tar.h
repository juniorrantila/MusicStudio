#pragma once
#include <Basic/Types.h>
#include <Basic/Allocator.h>
#include <Basic/FixedArena.h>

typedef enum e_tar : iptr {
    e_tar_none = 0,
    e_tar_invalid_file,
    e_tar_invalid_path,
    e_tar_invalid_file_size,
    e_tar_invalid_buffer_size,
    e_tar_no_mem,
} e_tar;

typedef struct TarCounter {
    u64 size;
} TarCounter;

typedef struct Tar {
    u8 const* base;
    u8 const* head;
    u8 const* seek;
    u8 const* end;
} Tar;

C_API c_string tar_strerror(e_tar error);

C_API bool untar(Tar*, char const** path, u64* path_size, u8 const** content, u64* content_size);

C_API Tar tar_init(void const* data, u64 size);
C_INLINE u64 tar_size(Tar tar) { return tar.end - tar.base; }
C_INLINE u8 const* tar_buffer(Tar tar) { return tar.base; }

C_API void tar_rewind(Tar*);
C_API bool tar_find(Tar const*, char const* path, u8 const** content, u64* content_size);
C_API bool tar_find2(Tar const*, char const* path, u64 path_size, u8 const** content, u64* content_size);

C_API TarCounter tar_counter(void);
C_API void tar_count(TarCounter*, u64 content_size);

C_API e_tar tar_add(Tar*, c_string path, void const* content, u64 content_size);
C_API e_tar tar_add2(Tar*, char const* path, u64 path_size, void const* content, u64 content_size);
