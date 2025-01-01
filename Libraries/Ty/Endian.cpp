#include "./Endian.h"

namespace Ty {

Endian device_endian()
{
    volatile u32 i = 0x01234567;
    if (*(u8*)&i == 0x67) {
        return Endian::Little;
    }
    return Endian::Big;
}

u16 device_endian_from_little(u16 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return value;
    case Endian::Big:
        return __builtin_bswap16(value);
    }
}

u32 device_endian_from_little(u32 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return value;
    case Endian::Big:
        return __builtin_bswap32(value);
    }
}

u64 device_endian_from_little(u64 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return value;
    case Endian::Big:
        return __builtin_bswap64(value);
    }
}

u16 device_endian_from_big(u16 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return __builtin_bswap16(value);
    case Endian::Big:
        return value;
    }
}

u32 device_endian_from_big(u32 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return __builtin_bswap32(value);
    case Endian::Big:
        return value;
    }
}

u64 device_endian_from_big(u64 value)
{
    switch (device_endian()) {
    case Endian::Little:
        return __builtin_bswap64(value);
    case Endian::Big:
        return value;
    }
}

}
