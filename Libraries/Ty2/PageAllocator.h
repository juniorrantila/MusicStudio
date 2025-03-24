#pragma once
#include "./Allocator.h"

C_API Allocator* page_allocator(void);

RETURNS_SIZED_BY(1)
C_API void* page_alloc(usize size) [[clang::allocating]];
C_API void page_free(void* ptr, usize size);
C_API usize page_size(void);
