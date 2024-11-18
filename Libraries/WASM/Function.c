#include "./Function.h"

#include "./Internal/Function.h"

c_string wasm_function_name(WASMFunction* function)
{
    switch (function->kind) {
    case WASMFunctionKind_Bytecode:
        return function->bytecode.name;
    case WASMFunctionKind_Native:
        return function->native.name;
    }
}

void wasm_function_call(WASMFunction* function) 
{
    switch (function->kind) {
    case WASMFunctionKind_Bytecode:
        // FIXME
        break;
    case WASMFunctionKind_Native:
        function->native.callback(function->context, function->native.user);
        break;
    }
}
