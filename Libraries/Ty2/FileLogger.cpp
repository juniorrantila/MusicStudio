#include "./FileLogger.h"
#include "./Logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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
        .is_tty = isatty(fileno(file)) == 1,
    };
}

static c_string level_string(LoggerEventTag tag, bool is_tty)
{
    if (is_tty) {
        switch (tag) {
        case LoggerEventTag_Debug: return CYAN "DEBUG" NORMAL;
        case LoggerEventTag_Info: return BLUE "INFO" NORMAL;
        case LoggerEventTag_Warning: return YELLOW "WARNING" NORMAL;
        case LoggerEventTag_Error: return RED "ERROR" NORMAL;
        case LoggerEventTag_Fatal: return RED "FATAL" NORMAL;
        }
    }
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
    int pid = getpid();

    auto t = time(0);
    struct tm tm;
    (void)gmtime_r(&t, &tm);
    char time_buf[1024];
    memset(time_buf, 0, sizeof(time_buf));
    (void)strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", &tm);

    (void)fprintf(file_logger->file, "%s[%d][%s]: %.*s\n",
        level_string(event.tag, file_logger->is_tty),
        pid,
        time_buf,
        len,
        event.message
    );
    if (event.tag == LoggerEventTag_Fatal) abort();
}
