#pragma once
#include "./Allocator.h"

C_API Allocator* page_allocator(void);

C_API void* page_alloc(u64 size) [[clang::allocating]];
C_API void page_free(void* ptr, u64 size);

C_API [[gnu::const]] u32 page_size(void);
