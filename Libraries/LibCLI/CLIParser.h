#pragma once
#include <Basic/Bits.h>
#include <Basic/StringSlice.h>
#include <Basic/Base.h>
#include <Basic/FixedArena.h>

typedef c_string cli_error;

constexpr u16 cli_arg_max = 64;

typedef struct CLIParser {
    StringSlice flag_long_names[cli_arg_max];
    StringSlice flag_short_names[cli_arg_max];
    StringSlice flag_explanations[cli_arg_max];
    void* flag_users[cli_arg_max];
    cli_error(*flags[cli_arg_max])(CLIParser*, void*);

    StringSlice option_long_names[cli_arg_max];
    StringSlice option_short_names[cli_arg_max];
    StringSlice option_placeholders[cli_arg_max];
    StringSlice option_explanations[cli_arg_max];
    void* option_users[cli_arg_max];
    cli_error(*options[cli_arg_max])(CLIParser*, void*, c_string);

    StringSlice positional_placeholders[cli_arg_max];
    StringSlice positional_explanations[cli_arg_max];
    void* positional_users[cli_arg_max];
    cli_error(*positionals[cli_arg_max])(CLIParser*, void*, c_string);

    void* context;
    c_string cached_usage;
    c_string* argv;
    int argc;

    u8 flag_count;
    u8 option_count;
    u8 positional_count;

    FixedArena arena;
    u8 arena_buffer[8 * KiB];

#if __cplusplus

    c_string usage();

    void add_flag(StringSlice long_name, StringSlice short_name, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user));
    void add_option(StringSlice long_name, StringSlice short_name, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user, c_string));
    void add_positional_argument(StringSlice placeholder, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user, c_string));

    [[nodiscard]] cli_error run(int argc, c_string argv[]);

#endif
} CLIParser;
static_assert(sizeof(CLIParser) == 20544);

C_API void cli_parser_init(CLIParser*, void* context);

C_API c_string cli_usage(CLIParser*);

C_API void cli_add_flag(CLIParser*, StringSlice long_name, StringSlice short_name, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user));

C_API void cli_add_option(CLIParser*, StringSlice long_name, StringSlice short_name, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user, c_string));

C_API void cli_add_positional_argument(CLIParser*, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*)(CLIParser*, void* user, c_string));

C_API [[nodiscard]] cli_error cli_run(CLIParser*, int argc, c_string argv[]);
