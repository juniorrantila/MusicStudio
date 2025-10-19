#pragma once
#include "./Forward.h"

#include <Basic/Types.h>
#include <Basic/Forward.h>

typedef struct TCPListener {
    int sock;
} TCPListener;

C_API KError tcp_listener_init(u16 port, int backlog, TCPListener*);
C_API KError tcp_listener_destroy(TCPListener*);

C_API KError tcp_accept(TCPListener const*, TCPClientConnection*);
