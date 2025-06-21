#pragma once
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
