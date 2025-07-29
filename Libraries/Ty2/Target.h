#pragma once

#if __aarch64__
#define TY_TARGET_ARCH "arm64"
#elif __x86_64__
#define TY_TARGET_ARCH "x86_64"
#elif __arm__
#define TY_TARGET_ARCH "arm"
#elif __i386__
#define TY_TARGET_ARCH "i386"
#else
#error "unknown architecture"
#endif

#ifndef TARGET_OS_LINUX
#ifdef __linux__
#define TARGET_OS_LINUX 1
#define TARGET_OS_MAC 0
#define TARGET_OS_WINDOWS 0
#endif
#endif

#ifndef TARGET_OS_MAC
#ifdef __APPLE__
#define TARGET_OS_LINUX 0
#define TARGET_OS_MAC 1
#define TARGET_OS_WINDOWS 0
#endif
#endif

#ifndef TARGET_OS_WINDOWS
#ifdef _WIN32
#define TARGET_OS_LINUX 0
#define TARGET_OS_MAC 0
#define TARGET_OS_WINDOWS 1
#endif
#endif

#if TARGET_OS_LINUX
#define TY_TARGET_ABI "gnu"
#define TY_TARGET_OS "linux"
#define TY_TARGET_SHARED_LIBRARY_PREFIX "lib"
#define TY_TARGET_SHARED_LIBRARY_EXTENSION "so"
#elif TARGET_OS_MAC
#define TY_TARGET_ABI "apple"
#define TY_TARGET_OS "darwin"
#define TY_TARGET_SHARED_LIBRARY_PREFIX "lib"
#define TY_TARGET_SHARED_LIBRARY_EXTENSION "dylib"
#elif TARGET_OS_WINDOWS
#define TY_TARGET_ABI "windows"
#define TY_TARGET_OS "windows"
#define TY_TARGET_SHARED_LIBRARY_PREFIX ""
#define TY_TARGET_SHARED_LIBRARY_EXTENSION "dll"
#else
#error "unknown os"
#endif

#define TY_TARGET_TRIPLE TY_TARGET_ARCH "-" TY_TARGET_ABI "-" TY_TARGET_OS

#define TY_BUILD_DIR "build"
#define TY_TARGET_BUILD_DIR TY_BUILD_DIR "/" TY_TARGET_TRIPLE

#define TY_TARGET_LIB_DIR TY_TARGET_BUILD_DIR "/lib"
#define TY_TARGET_BIN_DIR TY_TARGET_BUILD_DIR "/bin"

#define ty_library_path_c(name) TY_TARGET_LIB_DIR "/" TY_TARGET_SHARED_LIBRARY_PREFIX name "." TY_TARGET_SHARED_LIBRARY_EXTENSION
#define ty_library_path(name) ty_library_path_c(name) ""s
