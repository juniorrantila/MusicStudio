#pragma once
#include "./Forward.h"

#include "./Function.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WASMVirtualMachine WASMVirtualMachine;

WASMVirtualMachine* wasm_vm_create(WASMAllocator*);
void wasm_vm_destroy(WASMVirtualMachine*);

void* wasm_vm_variable(WASMVirtualMachine*, c_string name);
WASMFunction* wasm_vm_function(WASMVirtualMachine*, c_string name);

int wasm_vm_bind_variable(WASMVirtualMachine*, c_string, void* variable);
int wasm_vm_bind_function(WASMVirtualMachine*, c_string name, void* user, WASMNativeFunctionCallback callback);

int wasm_vm_push_i32(WASMVirtualMachine*, i32);
i32 wasm_vm_pop_i32(WASMVirtualMachine*);

int wasm_vm_push_u32(WASMVirtualMachine*, u32);
u32 wasm_vm_pop_u32(WASMVirtualMachine*);

int wasm_vm_push_f32(WASMVirtualMachine*, f32);
f32 wasm_vm_pop_f32(WASMVirtualMachine*);

void* wasm_vm_resolve_addr(WASMVirtualMachine*, u32);

#ifdef __cplusplus
}
#endif
