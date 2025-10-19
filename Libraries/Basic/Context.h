#pragma once
#include "./Types.h"
#include "./Forward.h"

#include <stdarg.h>

typedef struct Context Context;
typedef struct Context {
    Context* previous IF_CPP(= nullptr);

    Logger* log IF_CPP(= nullptr);
    FixedArena* temp_arena IF_CPP(= nullptr);
} Context;

C_INLINE void init_context(Context*);
C_API void init_default_context(c_string thread_name);
C_API void init_default_context_into(c_string thread_name, Context*);

C_API void push_context(Context*);
C_API void pop_context(void);

C_API void set_context(Context* context);
C_API Context* context(void);


C_API [[gnu::format(printf, 1, 2)]] void print(c_string, ...);
C_API [[gnu::format(printf, 1, 2)]] void debugf_(c_string, ...);
C_API [[gnu::format(printf, 1, 2)]] void infof_(c_string, ...);
C_API [[gnu::format(printf, 1, 2)]] void warnf_(c_string, ...);
C_API [[gnu::format(printf, 1, 2)]] void errorf_(c_string, ...);
C_API [[gnu::format(printf, 1, 2)]] void fatalf_(c_string, ...);

#ifdef NDEBUG
#define debugf debugf_
#define infof infof_
#define warningf warningf_
#define errorf errorf_
#define fatalf fatalf_
#else
#define debugf(fmt, ...) debugf_(" %16s:%u:  " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#define infof(fmt, ...)  infof_( " %16s:%u:  " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#define warnf(fmt, ...)  warnf_( " %16s:%u:  " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#define errorf(fmt, ...) errorf_(" %16s:%u:  " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#define fatalf(fmt, ...) fatalf_(" %16s:%u:  " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__) 
#endif

C_API [[gnu::format(printf, 1, 0)]] void vprint(c_string, va_list);
C_API [[gnu::format(printf, 1, 0)]] void vdebugf(c_string, va_list);
C_API [[gnu::format(printf, 1, 0)]] void vinfof(c_string, va_list);
C_API [[gnu::format(printf, 1, 0)]] void vwarnf(c_string, va_list);
C_API [[gnu::format(printf, 1, 0)]] void verrorf(c_string, va_list);
C_API [[gnu::format(printf, 1, 0)]] void vfatalf(c_string, va_list);

C_API [[gnu::format(printf, 1, 2)]] c_string tprint(c_string, ...);
C_API [[gnu::format(printf, 1, 0)]] c_string tvprint(c_string, va_list); 

C_API [[gnu::format(printf, 2, 3)]] KError tprints(StringSlice*, c_string, ...);
C_API [[gnu::format(printf, 2, 0)]] KError tvprints(StringSlice*, c_string, va_list); 

C_API Allocator* temporary_arena(void);
C_API void* tpush(u64 size, u64 align);
C_API void* tclone(void const*, u64 size, u64 align);
C_API void drain_temporary_arena(void);
C_API u64 tbytes_used(void);
C_API u64 tbytes_left(void);

#ifdef __cplusplus
template <typename T>
T* tclone(T value) { return (T*)tclone(&value, sizeof(T), alignof(T)); }
#endif
