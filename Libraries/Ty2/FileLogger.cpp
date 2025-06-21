#include "./FileLogger.h"
#include "./Logger.h"

#include <stdlib.h>
#include <unistd.h>

static void dispatch(struct Logger*, LoggerEvent);

#define RED    "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define CYAN   "\033[1;36m"
#define NORMAL "\033[0;0m"

C_API FileLogger make_file_logger(Allocator* temporary_arena, FILE* file)
{
    return (FileLogger){
        .logger = make_logger(temporary_arena, dispatch),
        .file = file,
        .name = nullptr,
        .is_tty = isatty(fileno(file)) == 1,
    };
}

static c_string severity(LoggerEventTag tag)
{
    switch (tag) {
    case LoggerEventTag_Debug: return "DEBG";
    case LoggerEventTag_Info: return "INFO";
    case LoggerEventTag_Warning: return "WARN";
    case LoggerEventTag_Error: return "ERRR";
    case LoggerEventTag_Fatal: return "FATAL";
    }
}

static c_string reset(bool is_tty)
{
    if (!is_tty) return "";
    return NORMAL;
}

static c_string color(LoggerEventTag tag, bool is_tty)
{
    if (!is_tty) return "";
    switch (tag) {
    case LoggerEventTag_Debug: return CYAN;
    case LoggerEventTag_Info: return BLUE;
    case LoggerEventTag_Warning: return YELLOW;
    case LoggerEventTag_Error:
    case LoggerEventTag_Fatal: return RED;
    }
}

static void dispatch(struct Logger* l, LoggerEvent event)
{
    FileLogger* file_logger = field_base(FileLogger, logger, l);
    int len = (int)event.message_size;
    int pid = getpid();

    bool is_tty = file_logger->is_tty;
    if (file_logger->name) {
        (void)fprintf(file_logger->file, "%s%s|%s%.2u%s|%s%u%s|%s%s%s%s: %.*s\n",
            color(event.tag, is_tty), // 1
            severity(event.tag),
            reset(is_tty),
            event.seq % 100,
            color(event.tag, is_tty),
            reset(is_tty),
            pid,
            color(event.tag, is_tty),
            reset(is_tty),
            file_logger->name,
            color(event.tag, is_tty),
            reset(is_tty),
            len,
            event.message
        );
    } else {
                                        // a b  c d e    f g    hi
        (void)fprintf(file_logger->file, "%s%s|%s%d%s|%.2X%s: %.*s\n",
            color(event.tag, is_tty), // a
            severity(event.tag),      // b
            reset(is_tty),            // c
            event.seq % 100,          // d
            color(event.tag, is_tty), // e
            pid,                      // f
            reset(is_tty),            // j
            len,
            event.message
        );
    }
    if (event.tag == LoggerEventTag_Fatal) abort();
}
