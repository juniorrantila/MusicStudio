#pragma once
#include <Basic/Types.h>
#include <Basic/Error.h>
#include <Basic/Allocator.h>

C_API KError core_read_entire_file(c_string, char const**, u64*, Allocator* IF_CPP(= temporary_arena()));
C_API KError core_write_entire_file(c_string, void const*, u64);
