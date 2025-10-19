#pragma once
#ifndef __cplusplus
#error "this file only works in C++ mode"
#endif

#include "./Base.h"

template <typename T>
struct Vector {
    Allocator* gpa;
    T* items;
    u64 count;
    u64 capacity;

    static Vector init(Allocator* gpa);
    void destroy();

    [[nodiscard]] bool append(T value);
};
