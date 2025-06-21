#pragma once
#include "./Base.h"

#include "./Allocator.h"
#ifdef __cplusplus
#include "./TypeId.h"
#endif

enum PromiseStatus : u8 {
    PromiseStatus_Garbage = 0,
    PromiseStatus_Pending,
    PromiseStatus_Resolved,
    PromiseStatus_Rejected,
};

typedef struct [[nodiscard]] { bool ok; } PromiseSuccess;
C_INLINE PromiseSuccess promise_ok() { return (PromiseSuccess){true}; }
C_INLINE PromiseSuccess promise_fail() { return (PromiseSuccess){false}; }

constexpr u64 promise_payload_size_max = 57;
constexpr u64 promise_payload_align_max = 16;
typedef struct [[nodiscard]] Promise {
    alignas(promise_payload_align_max) u8 data[promise_payload_size_max];
    u8 size;
    u8 align;
    _Atomic PromiseStatus status;
    u16 seq;
    u16 tag;

#ifdef __cplusplus
    void toss();
    bool is_ready() const;
    bool is_rejected() const { return status == PromiseStatus_Rejected; }
    bool is_resolved() const { return status == PromiseStatus_Resolved; }
    bool is_empty() const { return status == PromiseStatus_Garbage; }

    template <typename T>
        requires (sizeof(T) <= promise_payload_size_max) && (alignof(T) <= promise_payload_align_max)
    void reject(T const& value) { return reject(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    void reject(u16 tag, u64 data_size, u64 data_align, void const* data);
    void reject();

    template <typename T>
        requires (sizeof(T) <= promise_payload_size_max) && (alignof(T) <= promise_payload_align_max)
    void resolve(T const& value) { return resolve(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    void resolve(u16 tag, u64 data_size, u64 data_align, void const* data);
    void resolve();

    void* read_dynamic(u16 tag, Allocator* a) const;
    PromiseSuccess read(u16 tag, u64 size, u64 align, void*) const;

    template <typename T>
        requires (sizeof(T) <= promise_payload_size_max) && (alignof(T) <= promise_payload_align_max)
    T* read_dynamic(Allocator* a) const { return (T*)read(Ty2::type_id<T>(), a); }

    template <typename T>
        requires (sizeof(T) <= promise_payload_size_max) && (alignof(T) <= promise_payload_align_max)
    [[nodiscard]] PromiseSuccess read(T* buf) const
    {
        return read(Ty2::type_id<T>(), sizeof(T), alignof(T), buf);
    }

#endif
} Promise;
static_assert(sizeof(Promise) == 64);

C_INLINE Promise promise_empty(void) { return (Promise){}; }

C_API PromiseSuccess promise_init(Promise*, u16 seq, u16 tag, u64 data_size, u64 data_align, void const* data);

C_API bool promise_is_ready(Promise const*);
C_INLINE bool promise_is_rejected(Promise const* p) { return p->status == PromiseStatus_Rejected; }
C_INLINE bool promise_is_resolved(Promise const* p) { return p->status == PromiseStatus_Resolved; }
C_INLINE bool promise_is_empty(Promise const* p) { return p->status == PromiseStatus_Garbage; }

C_API void promise_toss(Promise*);

C_API void promise_reject(Promise*, u16 tag, u64 data_size, u64 data_align, void const* data);
C_API void promise_resolve(Promise*, u16 tag, u64 data_size, u64 data_align, void const* data);

C_API void* promise_read_dynamic(Promise const*, u16 tag, Allocator*);
C_API PromiseSuccess promise_read(Promise const*, u16 tag, u64 size, u64 align, void*);
