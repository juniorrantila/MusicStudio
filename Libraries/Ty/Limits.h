#pragma once
#include "Base.h"

namespace Ty {

template <typename T>
struct Limits {
    static T max();
    static T min();
};

template <>
struct Limits<i8> {
    static i8 max() { return 127; }
    static i8 min() { return -128; }
};

template <>
struct Limits<i16> {
    static i16 max() { return 32767; }
    static i16 min() { return -32768; }
};

template <>
struct Limits<i32> {
    static i32 max() { return 2147483647; }
    static i32 min() { return -2147483648; }
};

template <>
struct Limits<i64> {
    static i64 max() { return 9223372036854775807LL; }
    static i64 min() { return -9223372036854775808ULL; }
};

template <>
struct Limits<u8> {
    static u8 max() { return -1; }
    static u8 min() { return 0; }
};

template <>
struct Limits<u16> {
    static u16 max() { return -1; }
    static u16 min() { return 0; }
};

template <>
struct Limits<u32> {
    static u32 max() { return -1; }
    static u32 min() { return 0; }
};

template <>
struct Limits<u64> {
    static u64 max() { return -1; }
    static u64 min() { return 0; }
};

}
