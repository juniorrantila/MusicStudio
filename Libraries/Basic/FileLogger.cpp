#include "./FileLogger.h"

#include "./Logger.h"
#include "./Types.h"
#include "./Context.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void file_dispatch(struct Logger*, LoggerEvent);

#define RED    "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define CYAN   "\033[1;36m"
#define NORMAL "\033[0;0m"

C_API FileLogger file_logger_init(c_string name, FILE* file)
{
    return (FileLogger){
        .logger = logger_init(file_dispatch),
        .file = file,
        .name = name,
        .is_tty = isatty(fileno(file)) == 1,
    };
}

static c_string file_severity(LoggerEventTag tag)
{
    switch (tag) {
    case LoggerEventTag_Format: return "";
    case LoggerEventTag_Debug: return "DEBG";
    case LoggerEventTag_Info: return "INFO";
    case LoggerEventTag_Warning: return "WARN";
    case LoggerEventTag_Error: return "ERRR";
    case LoggerEventTag_Fatal: return "FATAL";
    }
}

static c_string file_reset(bool is_tty)
{
    if (!is_tty) return "";
    return NORMAL;
}

static c_string file_color(LoggerEventTag tag, bool is_tty)
{
    if (!is_tty) return "";
    switch (tag) {
    case LoggerEventTag_Format: return "";
    case LoggerEventTag_Debug: return CYAN;
    case LoggerEventTag_Info: return BLUE;
    case LoggerEventTag_Warning: return YELLOW;
    case LoggerEventTag_Error:
    case LoggerEventTag_Fatal: return RED;
    }
}

static void file_dispatch(struct Logger* l, LoggerEvent event)
{
    FileLogger* file_logger = FIELD_BASE(FileLogger, logger, l);
    if (event.tag == LoggerEventTag_Format) {
        (void)fwrite(event.message, event.message_size, 1, file_logger->file);
        (void)fflush(file_logger->file);
        return;
    }

    if (event.tag == LoggerEventTag_Debug) {
        if (getenv("DEBUG") == nullptr)
            return;
    }

    int len = (int)event.message_size;
    int pid = getpid();

    c_string name = file_logger->name;
    if (!name) name = context()->thread_name;
    bool is_tty = file_logger->is_tty;
    (void)fprintf(file_logger->file, "%s%s|%s%.2u%s|%s%u%s|%s%li%s|%s%16s%s|%s %.*s\n",
        file_color(event.tag, is_tty), // 1
        file_severity(event.tag),

        file_reset(is_tty),
        event.seq % 100,
        file_color(event.tag, is_tty),

        file_reset(is_tty),
        pid,
        file_color(event.tag, is_tty),

        file_reset(is_tty),
        kthread_index(event.originator),
        file_color(event.tag, is_tty),

        file_reset(is_tty),
        name,
        file_color(event.tag, is_tty),

        file_reset(is_tty),
        len,
        event.message
    );

    (void)fflush(file_logger->file);

    if (event.tag == LoggerEventTag_Fatal) abort();
}
