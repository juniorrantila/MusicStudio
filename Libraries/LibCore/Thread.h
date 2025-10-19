#pragma once
#include <Basic/Types.h>

C_API [[nodiscard]] bool core_thread_create(void*, void(*callback)(void*));
C_API void core_thread_set_name(c_string);
