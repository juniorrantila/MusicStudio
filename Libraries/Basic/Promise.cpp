#include "./Promise.h"

#include "./Verify.h"

#include <string.h>

void Promise::toss() { return promise_toss(this); }
C_API void promise_toss(Promise* promise)
{
    VERIFY(promise->is_ready());
    *promise = promise_empty();
}

bool Promise::is_ready() const { return promise_is_ready(this); }
C_API bool promise_is_ready(Promise const* p)
{
    switch (p->status) {
    case PromiseStatus_Garbage: UNREACHABLE();
    case PromiseStatus_Pending: return false;
    case PromiseStatus_Resolved:
    case PromiseStatus_Rejected: return true;
    }
}

C_API PromiseSuccess promise_init(Promise* promise, u16 seq, u16 tag, u64 data_size, u64 data_align, void const* data)
{
    if (verify(promise_is_empty(promise)).failed) return promise_fail();
    if (verify(data_size <= promise_payload_size_max).failed) return promise_fail();
    if (verify(data_align <= promise_payload_align_max).failed) return promise_fail();

    *promise = (Promise){
        .data = {},
        .size = (u8)data_size,
        .align = (u8)data_align,
        .status = PromiseStatus_Pending,
        .seq = seq,
        .tag = tag,
    };
    __builtin_memcpy(promise->data, data, data_size);

    return promise_ok();
}
 

void Promise::reject() { return reject(0, 0, 0, nullptr); }
void Promise::reject(u16 tag, u64 data_size, u64 data_align, void const* data) { return promise_reject(this, tag, data_size, data_align, data); }
C_API void promise_reject(Promise* promise, u16 tag, u64 data_size, u64 data_align, void const* data)
{
    VERIFY(data_size <= promise_payload_size_max);
    VERIFY(data_align <= promise_payload_align_max);
    VERIFY(promise->status == PromiseStatus_Pending);
    promise->tag = tag;
    promise->align = (u8)data_align;
    promise->size = (u8)data_size;
    memcpy(promise->data, data, data_size);
    promise->status = PromiseStatus_Rejected;
}


void Promise::resolve() { return resolve(0, 0, 0, nullptr); }
void Promise::resolve(u16 tag, u64 data_size, u64 data_align, void const* data) { return promise_resolve(this, tag, data_size, data_align, data); }
C_API void promise_resolve(Promise* promise, u16 tag, u64 data_size, u64 data_align, void const* data)
{
    VERIFY(data_size <= promise_payload_size_max);
    VERIFY(data_align <= promise_payload_align_max);
    VERIFY(promise->status == PromiseStatus_Pending);
    promise->tag = tag;
    promise->align = (u8)data_align;
    promise->size = (u8)data_size;
    memcpy(promise->data, data, data_size);
    promise->status = PromiseStatus_Resolved;
}


void* Promise::read_dynamic(u16 tag, Allocator* a) const { return promise_read_dynamic(this, tag, a); }
C_API void* promise_read_dynamic(Promise const* promise, u16 tag, Allocator* a)
{
    if (verify(promise->is_ready()).failed) return nullptr;
    if (verify(tag == promise->tag).failed) return nullptr;
    return memclone(a, promise->data, promise->size, promise->align);
}

PromiseSuccess Promise::read(u16 tag, u64 size, u64 align, void* buf) const { return promise_read(this, tag, size, align, buf); }
C_API PromiseSuccess promise_read(Promise const* promise, u16 tag, u64 size, u64 align, void* buf)
{
    if (verify(tag == promise->tag).failed) return promise_fail();
    if (verify(promise->size == size).failed) return promise_fail();
    if (verify(promise->align == align).failed) return promise_fail();
    memcpy(buf, promise->data, size);
    return promise_ok();
}
