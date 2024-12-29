#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if __x86_64__

#ifndef _WIN32
typedef struct AsyncContext {
    void* rip;
    void* rsp;
    void* rbx;
    void* rbp;
    void* r12;
    void* r13;
    void* r14;
    void* r15;
} AsyncContext;
#else
#error "unimplemented"
#endif

#elif __aarch64__

typedef struct AsyncContext {
    void* sp;
    void* lr;
    void* r19;
    void* r20;
    void* r21;
    void* r22;
    void* r23;
    void* r24;
    void* r25;
    void* r26;
    void* r27;
    void* r28;
    void* v8;
    void* v9;
    void* v10;
    void* v11;
    void* v12;
    void* v13;
    void* v14;
    void* v15;
} AsyncContext;

#else
#error "unknown architecture"
#endif

int async_context_save(AsyncContext*) __asm__("async_context_save");
void async_context_load(AsyncContext const*) __asm__("async_context_load");
void async_context_swap(AsyncContext* save, AsyncContext const* load) __asm__("async_context_swap");

#ifdef __cplusplus
}
#endif
