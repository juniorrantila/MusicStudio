#pragma once
#include <Basic/Logger.h>

#include "./MessageQueue.h"
#include "./Thread.h"

typedef struct THLogger {
    Logger logger;
    THMessageQueue queue;
    THThread io_thread;
    Logger* inner;
} THLogger;

C_API KError th_logger_init(THLogger*, Logger*);
C_API void th_logger_start(THLogger*);
