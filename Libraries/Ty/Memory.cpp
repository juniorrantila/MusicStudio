#include "Memory.h"
#include "ErrorOr.h"

namespace Ty {

ErrorOr<void*> allocate_memory(usize size)
{
    auto* ptr = __builtin_malloc(size);
    if (!ptr)
        return Error::from_errno();
    return ptr;
}

ErrorOr<void*> reallocate_memory(void* ptr, usize size)
{
    auto* new_ptr = __builtin_realloc(ptr, size);
    if (!new_ptr)
        return Error::from_errno();
    return new_ptr;
}

void free_memory(void* ptr) { __builtin_free(ptr); }

}
