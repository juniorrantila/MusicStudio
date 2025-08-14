#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunused"
#pragma clang attribute push (__attribute__((no_sanitize("integer"))), apply_to=function)

#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_DECORATE(x) stb_##x
#include "./stb_sprintf.h"

#pragma clang attribute pop
#pragma clang diagnostic pop
