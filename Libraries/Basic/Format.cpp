#include "./Format.h"

#include "./Verify.h"

#include <stb/sprintf.h>

C_API [[gnu::format(printf, 1, 2)]]
u64 format_size_including_null(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int len = stb_vsnprintf(nullptr, 0, fmt, args);
    VERIFY(len >= 0);

    va_end(args);
    return len + 1;
}
