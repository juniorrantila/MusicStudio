#pragma once
#include "./Allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

Allocator* page_allocator(void);

#ifdef __cplusplus
}
#endif
