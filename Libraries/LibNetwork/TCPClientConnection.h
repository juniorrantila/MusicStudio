#pragma once
#include "./Forward.h"

#include <Basic/Forward.h>
#include <Basic/Types.h>

typedef struct TCPClientConnection {
    int sock;
} TCPClientConnection;

C_API KError tcp_send(TCPClientConnection const*, void const*, u64);
C_API KError tcp_recv(TCPClientConnection const*, void*, u64);
C_API KError tcp_read_into(TCPClientConnection const*, StringBuilder*);
