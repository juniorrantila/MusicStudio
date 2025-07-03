#pragma once
#include "./Logger.h"

#include <stdio.h>

typedef struct FileLogger {
    Logger logger;
    FILE* file;
    c_string name;
    bool is_tty;

#ifdef __cplusplus
    Logger* operator->() { return &logger; }
#endif
} FileLogger;

C_API FileLogger file_logger_init(c_string name, FILE* file);
#ifdef __cplusplus
static inline FileLogger file_logger_init(FILE* file) { return file_logger_init(nullptr, file); }
#endif
