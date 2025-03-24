#pragma once
#include <Ty/Base.h>
#include <Ty2/Allocator.h>

#if __x86_64__
#define HOTRELOAD_ARCH "x86_64"
#elif __aarch64__
#define HOTRELOAD_ARCH "arm64"
#elif __i386__
#define HOTRELOAD_ARCH "i386"
#else
#error "unknown architecture"
#endif

#if __APPLE__
#define HOTRELOAD_ABI "apple"
#define HOTRELOAD_OS "darwin"
#elif __linux__
#define HOTRELOAD_ABI "gnu"
#define HOTRELOAD_OS "linux"
#elif _WIN32
#define HOTRELOAD_ABI "windows"
#define HOTRELOAD_OS "windows"
#else
#error "unknown ABI and OS"
#endif

#define HOTRELOAD_LIB_DIR "build/" HOTRELOAD_ARCH "-" HOTRELOAD_ABI "-" HOTRELOAD_OS "/lib"

typedef enum HotReloadTag {
    HotReloadTag_Size,
    HotReloadTag_Init,
    HotReloadTag_Deinit,
} HotReloadTag;

typedef struct {
    Allocator* gpa;
    usize size;
    void* state;
    HotReloadTag tag;
} HotReloadEvent;

typedef struct HotReload {
    void* (*dispatch)(HotReloadEvent);
} HotReload;

C_API usize hotreload_size(HotReload r);
C_API void* hotreload_init(HotReload r, Allocator* gpa, void* state, usize size);
C_API void* hotreload_deinit(HotReload r, Allocator* gpa, void* state, usize size);
