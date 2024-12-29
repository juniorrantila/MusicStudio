#include "wasm3.h"

#include <stdio.h>

#define FATAL(msg, ...) { printf("Fatal: " msg "\n", ##__VA_ARGS__); return 1; }

unsigned char a_out[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
  0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02, 0x6d, 0x73,
  0x05, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x00, 0x00, 0x03, 0x02, 0x01, 0x01,
  0x05, 0x03, 0x01, 0x00, 0x02, 0x06, 0x08, 0x01, 0x7f, 0x01, 0x41, 0x90,
  0x88, 0x04, 0x0b, 0x07, 0x13, 0x02, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72,
  0x79, 0x02, 0x00, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x01,
  0x0a, 0x10, 0x01, 0x0e, 0x00, 0x41, 0x80, 0x88, 0x80, 0x80, 0x00, 0x10,
  0x80, 0x80, 0x80, 0x80, 0x00, 0x0b, 0x0b, 0x15, 0x01, 0x00, 0x41, 0x80,
  0x08, 0x0b, 0x0e, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x57, 0x6f,
  0x72, 0x6c, 0x64, 0x21, 0x00
};
unsigned int a_out_len = 113;


const void* print(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    void* user = context->userdata;
    uint64_t location = *stack;
    uint64_t size = m3_GetMemorySize(runtime);
    if (location >= size) {
        return m3Err_trapOutOfBoundsMemoryAccess;
    }
    char const* s = &((char*)memory)[location];
    printf("%p %s\n", user, s);
    return m3Err_none;
}

int main(void)
{
    M3Result result = m3Err_none;

    uint8_t* wasm = (uint8_t*)a_out;
    uint32_t fsize = a_out_len;

    printf("Loading WebAssembly...\n");
    IM3Environment env = m3_NewEnvironment ();
    if (!env) FATAL("m3_NewEnvironment failed");

    IM3Runtime runtime = m3_NewRuntime (env, 1024, NULL);
    if (!runtime) FATAL("m3_NewRuntime failed");

    IM3Module module;
    result = m3_ParseModule (env, &module, wasm, fsize);
    if (result) FATAL("m3_ParseModule: %s", result);

    result = m3_LoadModule (runtime, module);
    if (result) FATAL("m3_LoadModule: %s", result);

    result = m3_LinkRawFunctionEx(module, "ms", "print", "v(*)", print, (void*)1);
    if (result) FATAL("m3_LinkRawFunctionEx: %s", result);

    result = m3_CompileModule(module);
    if (result) FATAL("m3_CompileModule: %s", result);

    IM3Function f;
    result = m3_FindFunction(&f, runtime, "_start");
    if (result) FATAL("m3_FindFunction: %s", result);

    printf("Running...\n");

    result = m3_Call(f, 0, 0);
    if (result) FATAL("m3_Call: %s", result);
}
