#pragma once
#include "./Base.h"

namespace Ty {

enum class Endian {
    Little,
    Big,
};

Endian device_endian();

u16 device_endian_from_little(u16);
u32 device_endian_from_little(u32);
u64 device_endian_from_little(u64);

u16 device_endian_from_big(u16);
u32 device_endian_from_big(u32);
u64 device_endian_from_big(u64);

}
