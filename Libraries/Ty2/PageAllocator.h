#pragma once
#include "./Allocator.h"

C_API Allocator* page_allocator(void);

C_API void* page_alloc(usize size);
C_API void page_free(void* ptr, usize size);
C_API usize page_size(void);
