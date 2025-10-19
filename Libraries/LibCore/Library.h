#pragma once
#include <Basic/Types.h>
#include <Basic/StringSlice.h>

typedef struct Library Library;

C_API Library* library_create_from_memory(c_string identifier, void* mem, u64 size);
C_API Library* library_create_from_path(char const* path);
C_API void library_destroy(Library*);

C_API void* library_get_symbol(Library const* library, c_string name);
