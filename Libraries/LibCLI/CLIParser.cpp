#include "./CLIParser.h"

#include <Basic/StringSlice.h>
#include <Basic/Verify.h>
#include <Basic/FixedArena.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool is_running(CLIParser* p) { return p->argv != 0 || p->argc != 0; }

C_API void cli_parser_init(CLIParser* parser, void* context)
{
    *parser = (CLIParser){
        .flag_long_names = {},
        .flag_short_names = {},
        .flag_explanations = {},
        .flag_users = {},
        .flags = {},
        .option_long_names = {},
        .option_short_names = {},
        .option_placeholders = {},
        .option_explanations = {},
        .option_users = {},
        .options = {},
        .positional_placeholders = {},
        .positional_explanations = {},
        .positional_users = {},
        .positionals = {},
        .context = context,
        .cached_usage = nullptr,
        .argv = nullptr,
        .argc = 0,
        .flag_count = 0,
        .option_count = 0,
        .positional_count = 0,
        .arena = {},
        .arena_buffer = {},
    };
    parser->arena = fixed_arena_init(parser->arena_buffer, sizeof(parser->arena_buffer));

    parser->add_flag("--help"s, "-h"s, "show this message"s, nullptr, [](CLIParser* parser, void*) -> cli_error {
        puts(parser->usage());
        exit(0);
        return nullptr;
    });
}

c_string CLIParser::usage() { return cli_usage(this); }
C_API c_string cli_usage(CLIParser* parser)
{
    if (parser->cached_usage)
        return parser->cached_usage;
    VERIFYS(is_running(parser), "run() has not been called");

    c_string program_name = parser->argv[0];
    auto* arena = &parser->arena;

    parser->cached_usage = arena->must_fmt("USAGE: %s %s", program_name, parser->option_count == 0 ? "[flags] " : "[flags|options] ");
    arena->head -= 1;
    for (int i = 0; i < parser->positional_count; i++) {
        auto placeholder = parser->positional_placeholders[i];
        arena->must_fmt("%.*s ", (int)placeholder.count, placeholder.items);
        arena->head -= 1;
    }
    arena->must_fmt("\n\n");
    arena->head -= 1;
    arena->must_fmt("FLAGS:\n");
    arena->head -= 1;
    for (int i = 0; i < parser->flag_count; i++) {
        auto short_name = parser->flag_short_names[i];
        auto long_name = parser->flag_long_names[i];
        auto explanation = parser->flag_explanations[i];
        c_string pad = short_name.count == 2 ? " " : "";
        u64 bytes = strlen(arena->must_fmt("        %.*s, %s%.*s", (int)short_name.count, short_name.items, pad, (int)long_name.count, long_name.items));
        arena->head -= 1;
        for (; bytes < 40; bytes++) {
            arena->must_fmt(" ");
            arena->head -= 1;
        }
        arena->must_fmt("%.*s\n", (int)explanation.count, explanation.items);
        arena->head -= 1;
    }
    if (parser->option_count != 0) {
        arena->must_fmt("\n");
        arena->head -= 1;
        arena->must_fmt("OPTIONS:\n");
        arena->head -= 1;
        for (int i = 0; i < parser->option_count; i++) {
            auto short_name = parser->option_short_names[i];
            auto long_name = parser->option_long_names[i];
            auto placeholder = parser->option_placeholders[i];
            auto explanation = parser->option_explanations[i];
            c_string pad = short_name.count == 2 ? " " : "";
            u64 bytes = strlen(arena->must_fmt("        %.*s, %s%.*s  <%.*s> ", (int)short_name.count, short_name.items, pad, (int)long_name.count, long_name.items, (int)placeholder.count, placeholder.items));
            arena->head -= 1;
            for (; bytes < 40; bytes++) {
                arena->must_fmt(" ");
                arena->head -= 1;
            }
            arena->must_fmt("%.*s\n", (int)explanation.count, explanation.items);
            arena->head -= 1;
        }
    }
    arena->must_fmt("");

    return parser->cached_usage;
}

void CLIParser::add_flag(StringSlice long_name, StringSlice short_name, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user)) { return cli_add_flag(this, long_name, short_name, explanation, user, callback); }
C_API void cli_add_flag(CLIParser* parser, StringSlice long_name, StringSlice short_name, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user))
{
    VERIFYS(!is_running(parser), "you may not add flags after run() has been called");
    u8 id = parser->flag_count++;
    VERIFY(id < cli_arg_max);
    parser->flag_long_names[id] = long_name;
    parser->flag_short_names[id] = short_name;
    parser->flag_explanations[id] = explanation;
    parser->flag_users[id] = user;
    parser->flags[id] = callback;
}

void CLIParser::add_option(StringSlice long_name, StringSlice short_name, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user, c_string)) { return cli_add_option(this, long_name, short_name, placeholder, explanation, user, callback); }
C_API void cli_add_option(CLIParser* parser, StringSlice long_name, StringSlice short_name, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user, c_string))
{
    VERIFYS(!is_running(parser), "you may not add options after run() has been called");
    u8 id = parser->option_count++;
    VERIFY(id < cli_arg_max);
    parser->option_long_names[id] = long_name;
    parser->option_short_names[id] = short_name;
    parser->option_placeholders[id] = placeholder;
    parser->option_explanations[id] = explanation;
    parser->option_users[id] = user;
    parser->options[id] = callback;
}

void CLIParser::add_positional_argument(StringSlice placeholder, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user, c_string)) { return cli_add_positional_argument(this, placeholder, explanation, user, callback); }
C_API void cli_add_positional_argument(CLIParser* parser, StringSlice placeholder, StringSlice explanation, void* user, cli_error(*callback)(CLIParser*, void* user, c_string))
{
    VERIFYS(!is_running(parser), "you may not add positional arguments after run() has been called");
    u8 id = parser->positional_count++;
    VERIFY(id < cli_arg_max);
    parser->positional_placeholders[id] = placeholder;
    parser->positional_explanations[id] = explanation;
    parser->positional_users[id] = user;
    parser->positionals[id] = callback;
}

[[nodiscard]] static bool flag_find(CLIParser*, StringSlice, u32*);
[[nodiscard]] static bool option_find(CLIParser*, StringSlice, u32*);

[[nodiscard]] cli_error CLIParser::run(int argc, c_string argv[]) { return cli_run(this, argc, argv); }
C_API [[nodiscard]] cli_error cli_run(CLIParser* parser, int argc, c_string argv[])
{
    parser->argc = argc;
    parser->argv = argv;

    auto* arena = &parser->arena;

    u32 used_positionals = 0;
    for (int arg = 1; arg < argc; arg++) {
        auto argument = sv_from_c_string(argv[arg]);

        u32 flag;
        if (flag_find(parser, argument, &flag)) {
            if (cli_error error = parser->flags[flag](parser, parser->flag_users[flag]); error != nullptr)
                return error;
            continue;
        }

        u32 option;
        if (option_find(parser, argument, &option)) {
            if (arg + 1 >= argc)
                return arena->must_fmt("No argument provided for option '%s'\n\nSee help for more info (%s --help)\n", argv[arg], argv[0]);
            if (cli_error error = parser->options[option](parser, parser->option_users[option], argv[++arg]); error != nullptr)
                return error;
            continue;
        }

        if (used_positionals < parser->positional_count) {
            auto id = used_positionals++;
            if (cli_error error = parser->positionals[id](parser, parser->positional_users[id], argv[arg]); error != nullptr)
                return error;
            continue;
        }

        return arena->must_fmt("Unrecognized argument: '%s'\n\nSee help for more info (%s --help)\n", argv[arg], argv[0]);
    }

    if (used_positionals != parser->positional_count) {
        c_string base = nullptr;
        if (parser->positional_count - used_positionals == 1) {
            auto placeholder = parser->positional_placeholders[used_positionals];
            base = arena->must_fmt("Missing positional argument: %.*s\n", (int)placeholder.count, placeholder.items);
            arena->head -= 1;
        } else {
            base = arena->must_fmt("Missing positional arguments:\n\n");
            arena->head -= 1;
            for (u32 i = used_positionals; i < parser->positional_count; i++) {
                auto placeholder = parser->positional_placeholders[i];
                arena->must_fmt("        - %.*s\n", (int)placeholder.count, placeholder.items);
                arena->head -= 1;
            }
        }

        arena->must_fmt("\nSee help for more info (%s --help)\n", argv[0]);
        return base;
    }

    return nullptr;
}

[[nodiscard]] static bool flag_find(CLIParser* parser, StringSlice flag, u32* out)
{
    for (u32 i = 0; i < parser->flag_count; i++) {
        if (sv_equal(flag, parser->flag_short_names[i]))
            return *out = i, true;
        if (sv_equal(flag, parser->flag_long_names[i]))
            return *out = i, true;
    }
    return false;
}

[[nodiscard]] static bool option_find(CLIParser* parser, StringSlice option, u32* out)
{
    for (u32 i = 0; i < parser->flag_count; i++) {
        if (sv_equal(option, parser->option_short_names[i]))
            return *out = i, true;
        if (sv_equal(option, parser->option_long_names[i]))
            return *out = i, true;
    }
    return false;
}
