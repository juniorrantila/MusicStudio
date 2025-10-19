#pragma once
#include "./Allocator.h"

C_API Allocator* page_allocator(void);

C_API void* page_alloc(u64 size) [[clang::allocating]];
C_API void page_free(void* ptr, u64 size);

C_API [[gnu::const]] u64 page_size(void);
