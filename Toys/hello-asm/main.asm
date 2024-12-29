#ifdef __aarch64__
#include "./main.arm64"
#elif __x86_64__
#include "./main.x86"
#else
#error "unimplemented"
#endif
