#pragma once
#include "./Forward.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*WASMNativeFunctionCallback)(WASMVirtualMachine*, void*);

void wasm_function_call(WASMFunction*);
c_string wasm_function_name(WASMFunction*);

#ifdef __cplusplus
}
#endif
