#pragma once
#include "Base.h"

constexpr void* operator new(usize, void* addr) { return addr; }
