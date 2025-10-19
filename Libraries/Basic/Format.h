#pragma once
#include "./Types.h"

C_API [[gnu::format(printf, 1, 2)]]
u64 format_size_including_null(c_string fmt, ...);
