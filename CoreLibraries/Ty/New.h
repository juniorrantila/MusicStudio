#pragma once
#include "Base.h"

#ifdef __linux__

#include <new> // Ubuntu seems to include this somewhere...

#else

constexpr void* operator new(usize, void* addr) { return addr; }

#endif
