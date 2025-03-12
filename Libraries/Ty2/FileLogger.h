#pragma once
#include "./Logger.h"
#include <stdio.h>

typedef struct {
    Logger logger;
    FILE* file;
} FileLogger;

C_API FileLogger make_file_logger(Allocator* temporary_arena, FILE* file);
