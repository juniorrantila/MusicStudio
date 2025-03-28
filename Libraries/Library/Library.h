#pragma once
#include "./HotReload.h"

#include <Ty/Base.h>
#include <Ty/StringSlice.h>
#include <FS/FSVolume.h>

typedef struct Library Library;
typedef enum e_library {
#define LIBRARY_ERROR(ident, ...) e_library_##ident,
#include "./Error.def"
#undef LIBRARY_ERROR
} e_library;

C_API c_string library_strerror(e_library error);

C_API Library* library_create_from_memory(c_string identifier, void* mem, usize size, e_library* error);
C_API Library* library_create_from_path(char const* path, e_library* error);
C_API void library_destroy(Library*);

C_API void* library_get_symbol(Library const* library, c_string name);

C_API Library* library_hotreloadable(Allocator* gpa, FSVolume*, FileID, c_string dispatch_symbol, e_library* error);
C_API void library_update(Library*);
C_API void* library_hotreload_state(Library const*);
C_API usize library_hotreload_state_size(Library const*);
C_API usize library_hotreload_state_align(Library const*);
C_API HotReload library_hotreload(Library const*);

C_API bool library_needs_reload(Library const*);
C_API bool library_reload(Library*);
