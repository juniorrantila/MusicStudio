#pragma once
#include "../VirtualMachine.h"
#include "./Function.h"

typedef struct {
    c_string name;
    void* address;
} WASMNativeVariable;

struct WASMVirtualMachine {
    WASMAllocator* allocator;

    struct {
        u32 count;
        u32 capacity;
        WASMNativeVariable* items;
    } native_variables;

    struct {
        u32 count;
        u32 capacity;
        WASMFunction* items;
    } functions;

    struct {
        u32 count;
        u32 capacity;
        u32* items;
    } stack;

    u8 memory[16ULL * 1024ULL];
};
