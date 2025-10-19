#pragma once
#include "./Forward.h"

#include <Basic/Types.h>
#include <Basic/Allocator.h>
#include <Basic/Error.h>

typedef struct WASMVirtualMachine WASMVirtualMachine;
typedef struct WASMModule WASMModule;

typedef KError(*WASMNativeFunctionCallback)(WASMVirtualMachine*);
typedef struct WASMFunctionID { u32 index; } WASMFunctionID;

C_API u64 wasm_mod_min_size(void);
C_API u64 wasm_mod_min_align(void);
C_API KError wasm_mod_init(WASMModule**, void* module_memory, u64 module_memory_size, u64 module_memory_align, void const* wasm_data, u64 wasm_size);

C_API KError wasm_mod_bind_variable(WASMModule*, c_string, void* variable);
C_API KError wasm_mod_bind_function(WASMModule*, c_string mod, c_string name, c_string signature, WASMNativeFunctionCallback callback);

C_API u64 wasm_vm_min_size(void);
C_API u64 wasm_vm_min_align(void);
C_API KError wasm_vm_init(WASMVirtualMachine**, void* vm_memory, u64 vm_memory_size, u64 vm_memory_align, WASMModule const* module);

C_API void* wasm_vm_variable(WASMVirtualMachine*, c_string name);
C_API KError wasm_vm_function(WASMVirtualMachine*, c_string name, WASMFunctionID* out);
C_API void wasm_vm_set_user_context(WASMVirtualMachine*, void*);
C_API void* wasm_vm_user_context(WASMVirtualMachine*);

C_API KError wasm_vm_call(WASMVirtualMachine*, WASMFunctionID);

C_API KError wasm_vm_push_i32(WASMVirtualMachine*, i32);
C_API KError wasm_vm_push_u32(WASMVirtualMachine*, u32);
C_API KError wasm_vm_push_f32(WASMVirtualMachine*, f32);
#define wasm_vm_push(vm, value) _Generic((value),   \
            i32: wasm_vm_push_i32,                  \
            u32: wasm_vm_push_u32,                  \
            f32: wasm_vm_push_f32                   \
        )(vm, value)

C_API KError wasm_vm_pop_i32(WASMVirtualMachine*, i32*);
C_API KError wasm_vm_pop_u32(WASMVirtualMachine*, u32*);
C_API KError wasm_vm_pop_f32(WASMVirtualMachine*, f32*);
#define wasm_vm_pop(vm, dest) _Generic((*dest), \
            i32: wasm_vm_pop_i32,               \
            u32: wasm_vm_pop_u32,               \
            f32: wasm_vm_pop_f32                \
        )(vm, dest)


C_API void* wasm_vm_resolve_addr(WASMVirtualMachine*, u32, u32* size_until_end);
