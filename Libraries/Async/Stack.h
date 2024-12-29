#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* async_stack_swap(void*) __asm__("async_stack_swap");

#ifdef __cplusplus
}
#endif
