#include "./RPCServer.h"

#include "./Verify.h"
#include "./Mailbox.h"

#include <errno.h>

C_API RPCServer rpc_server_init(Mailbox* worker)
{
    auto s = (RPCServer){
        .worker = worker,
        .seq = 0,
        .items = {},
    };
    for (u64 i = 0; i < rpc_capacity; i++)
        s.items[i] = promise_empty();
    return s;
}

Promise* RPCServer::request(u16 tag, u64 data_size, u64 data_align, void const* data) { return rpc_request(this, tag, data_size, data_align, data); }
C_API Promise* rpc_request(RPCServer* s, u16 tag, u64 data_size, u64 data_align, void const* data)
{
    if (verify(data_size <= promise_payload_size_max).failed) return nullptr;
    if (verify(data_align <= promise_payload_align_max).failed) return nullptr;

    u16 seq = s->seq;
    u16 slot = seq % rpc_capacity;
    Promise* promise = &s->items[slot];
    if (verify(promise_is_empty(promise)).failed) return nullptr;
    if (verify(promise_init(promise, seq, tag, data_size, data_align, data).ok).failed) return nullptr;
    if (verify(s->worker->writer()->post(RPCRequest(promise)).ok).failed) {
        *promise = promise_empty();
        return nullptr;
    }

    s->seq += 1;
    return promise;
}


KError RPCServer::signal(u16 tag, u64 data_size, u64 data_align, void const* data) { return rpc_signal(this, tag, data_size, data_align, data); }
C_API KError rpc_signal(RPCServer* s, u16 tag, u64 data_size, u64 data_align, void const* data)
{
    if (verify(data_size <= message_size_max).failed) return kerror_unix(EINVAL);
    if (verify(data_align <= message_align_max).failed) return kerror_unix(EINVAL);
    return s->worker->writer()->post(tag, data_size, data_align, data);
}
