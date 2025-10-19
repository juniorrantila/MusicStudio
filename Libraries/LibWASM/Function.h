#pragma once
#include "./Forward.h"

#include <Basic/Types.h>
#include <Basic/Error.h>

typedef KError(*WASMNativeFunctionCallback)(WASMVirtualMachine*, void*);

C_API KError wasm_function_call(WASMFunction*);
C_API c_string wasm_function_name(WASMFunction*);
