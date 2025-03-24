#pragma once
#include "./Logger.h"
#include <stdio.h>

typedef struct {
    Logger logger;
    FILE* file;
    bool is_tty;
} FileLogger;

C_API FileLogger make_file_logger(Allocator* temporary_arena, FILE* file);
