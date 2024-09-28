#include "./Instrumentation.h"
#include <dlfcn.h>
#include <stdlib.h>

extern "C" {

char *__cxa_demangle(char const* mangled_name, char* output_buffer, size_t* length, int* status);

static int level = 0;

[[gnu::no_instrument_function]]
static void indent()
{
    for (int i = 0; i < level; i++) {
        __builtin_printf("  ");
    }
}

[[gnu::no_instrument_function]] void __cyg_profile_func_enter(void* func, void* caller);
void __cyg_profile_func_enter(void* func, void* caller)
{
    if (!Debug::Instrumentation::enabled) {
        return;
    }
    (void)caller;
    Dl_info info;
    dladdr(func, &info);
    indent();
    level++;

    int status = 0;
    size_t size = 0;
    char* name = __cxa_demangle(info.dli_sname, 0, &size, &status);
    __builtin_printf("%.*s {\n", (int)size, status == 0 ? name : info.dli_sname);
    if (status == 0) {
        free(name);
    }
}

[[gnu::no_instrument_function]] void __cyg_profile_func_exit(void* func, void* caller);
void __cyg_profile_func_exit(void* func, void* caller)
{
    if (!Debug::Instrumentation::enabled) {
        return;
    }
    (void)func;
    (void)caller;
    --level;
    if (level < 0) {
        level = 0;
        return;
    }
    indent();
    __builtin_printf("}\n");
}

}
