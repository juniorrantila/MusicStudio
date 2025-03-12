#include "./FileLogger.h"
#include "./Logger.h"

#include <stdlib.h>
#include <unistd.h>

static void dispatch(struct Logger*, LoggerEvent);

C_API FileLogger make_file_logger(Allocator* temporary_arena, FILE* file)
{
    return (FileLogger){
        .logger = make_logger(temporary_arena, dispatch),
        .file = file,
    };
}

static c_string level_string(LoggerEventTag tag)
{
    switch (tag) {
    case LoggerEventTag_Debug: return "DEBUG";
    case LoggerEventTag_Info: return "INFO";
    case LoggerEventTag_Warning: return "WARNING";
    case LoggerEventTag_Error: return "ERROR";
    case LoggerEventTag_Fatal: return "FATAL";
    }
}

static void dispatch(struct Logger* l, LoggerEvent event)
{
    FileLogger* file_logger = field_base(FileLogger, logger, l);
    int len = (int)event.message_size;
    (void)fprintf(file_logger->file, "%s: %.*s\n", level_string(event.tag), len, event.message);
    if (event.tag == LoggerEventTag_Fatal) abort();
}
