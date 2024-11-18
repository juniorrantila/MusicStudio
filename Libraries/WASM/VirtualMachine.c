#include "./VirtualMachine.h"

#include "./Allocator.h"
#include "./util.h"
#include "./Internal/Function.h"
#include "./Internal/VirtualMachine.h"

#include <errno.h>
#include <string.h>

WASMVirtualMachine* wasm_vm_create(WASMAllocator* a)
{
    WASMVirtualMachine* context = allocator_alloc(a, WASMVirtualMachine);
    memset(context, 0, sizeof(*context));
    context->allocator = a;
    return context;
}

void wasm_vm_destroy(WASMVirtualMachine* context)
{
    memset(context, 0, sizeof(*context));
    allocator_free(context->allocator, context); 
}

static int wasm_push_b32(WASMVirtualMachine* vm, void* value)
{
    return VEC_PUSH(vm->allocator, &vm->stack, *(u32*)value);
}

static void* wasm_pop_b32(WASMVirtualMachine* vm)
{
    if (vm->stack.count == 0) {
        // FIXME
        return 0;
    }
    return &vm->stack.items[--vm->stack.count];
}

int wasm_vm_push_i32(WASMVirtualMachine* vm, i32 value)
{
    return wasm_push_b32(vm, &value);
}

i32 wasm_vm_pop_i32(WASMVirtualMachine* vm)
{
    return *(i32*)wasm_pop_b32(vm);
}

int wasm_vm_push_u32(WASMVirtualMachine* vm, u32 value)
{
    return wasm_push_b32(vm, &value);
}

u32 wasm_vm_pop_u32(WASMVirtualMachine* vm)
{
    return *(u32*)wasm_pop_b32(vm);
}

int wasm_vm_push_f32(WASMVirtualMachine* vm, f32 value)
{
    return wasm_push_b32(vm, &value);
}

f32 wasm_vm_pop_f32(WASMVirtualMachine* vm)
{
    return *(f32*)wasm_pop_b32(vm);
}

WASMFunction* wasm_vm_function(WASMVirtualMachine* vm, c_string name)
{
    for (usize i = 0; i < vm->functions.count; i++) {
        WASMFunction* function = &vm->functions.items[i];
        if (function == 0)
            continue;
        if (strcmp(wasm_function_name(function), name) == 0) {
            return function;
        }
    }
    errno = EINVAL;
    return 0;
}

int wasm_vm_bind_variable(WASMVirtualMachine* vm, c_string name, void* variable)
{
    return VEC_PUSH(vm->allocator, &vm->native_variables, (WASMNativeVariable) {
        .name = name,
        .address = variable
    });
}

int wasm_vm_bind_function(WASMVirtualMachine* vm, c_string name, void* user, WASMNativeFunctionCallback callback)
{
    return VEC_PUSH(vm->allocator, &vm->functions, (WASMFunction) {
        .context = vm,
        .native = {
            .name = name,
            .callback = callback,
            .user = user,
        },
        .kind = WASMFunctionKind_Native,
    });
}

void* wasm_vm_variable(WASMVirtualMachine* vm, c_string name)
{
    for (usize i = 0; i < vm->native_variables.count; i++) {
        WASMNativeVariable* var = &vm->native_variables.items[i];
        if (strcmp(var->name, name) == 0) {
            return var->address;
        }
    }
    errno = EINVAL;
    return 0;
}

void* wasm_vm_resolve_addr(WASMVirtualMachine* context, u32 addr)
{
    return &context->memory[addr];
}
