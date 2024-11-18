#pragma once
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Library Library;
typedef enum e_library {
#define LIBRARY_ERROR(ident, ...) e_library_##ident,
#include "./Error.def"
#undef LIBRARY_ERROR
} e_library;

c_string library_strerror(e_library error);

Library* library_create_from_memory(c_string identifier, void* mem, usize size, e_library* error);
Library* library_create_from_path(char const* path, e_library* error);
void library_destroy(Library*);

void* library_get_symbol(Library const* library, c_string name);

#ifdef __cplusplus
}
#endif
