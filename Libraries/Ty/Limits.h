#pragma once
#include "Base.h"

namespace Ty {

template <typename T>
struct Limits {
    static T max();
    static T min();
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
