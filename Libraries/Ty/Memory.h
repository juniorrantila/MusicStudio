#pragma once
#include "Forward.h"

#include "Base.h"

namespace Ty {

ErrorOr<void*> allocate_memory(usize size);
ErrorOr<void*> reallocate_memory(void* ptr, usize size);
void free_memory(void* ptr);

}
