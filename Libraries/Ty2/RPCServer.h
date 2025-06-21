#pragma once
#include "./Base.h"
#include "./Mailbox.h"
#include "./Promise.h"

typedef struct RPCServerRequest {
    Promise* promise;
} RPCServerRequest;

static constexpr u64 rpc_capacity = 1024;
typedef struct RPCServer {
    Mailbox* worker;

    u16 seq;
    Promise items[rpc_capacity];

#ifdef __cplusplus
    template <typename T>
    Promise* request(T const& value) { return request(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    Promise* request(u16 tag, u64 data_size, u64 data_align, void const* data);

    template <typename T>
    MailboxSuccess signal(T const& value) { return signal(Ty2::type_id<T>(), sizeof(T), alignof(T), &value); }
    MailboxSuccess signal(u16 tag, u64 data_size, u64 data_align, void const* data);
#endif
} RPCServer;

C_API RPCServer rpc_server_init(Mailbox* worker);
C_API Promise* rpc_server_request(RPCServer* s, u16 tag, u64 data_size, u64 data_align, void const* data);
C_API MailboxSuccess rpc_server_signal(RPCServer* s, u16 tag, u64 data_size, u64 data_align, void const* data);
