#pragma once

typedef struct KError KError;
typedef struct WASMVirtualMachine WASMVirtualMachine;
typedef KError(*WASMNativeFunctionCallback)(WASMVirtualMachine*);
