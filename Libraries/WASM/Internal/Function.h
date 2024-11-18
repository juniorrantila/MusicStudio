#pragma once
#include "../Function.h"

typedef enum {
    WASMFunctionKind_Native,
    WASMFunctionKind_Bytecode,
} WASMFunctionKind;

typedef struct {
    c_string name;
    WASMNativeFunctionCallback callback;
    void* user;
} WASMNativeFunction;

typedef struct {
    c_string name;
    usize bytecode_size;
    u8 const* bytecode;
} WASMBytecodeFunction;

typedef struct WASMFunction {
    WASMVirtualMachine* context;
    union {
        WASMNativeFunction native;
        WASMBytecodeFunction bytecode;
    };
    WASMFunctionKind kind;
} WASMFunction;
