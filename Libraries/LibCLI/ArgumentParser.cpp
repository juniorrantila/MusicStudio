#include "./ArgumentParser.h"

#include <Basic/Array.h>
#include <Basic/Context.h>
#include <Basic/FileLogger.h>

#include <stdlib.h>

namespace CLI {

ArgumentParser::ArgumentParser()
{
    MUST(add_flag("--help"sv, "-h"sv, "show this message"sv, [this] {
        this->print_usage_and_exit();
    }));
}

ErrorOr<void> ArgumentParser::add_flag(StringView long_name,
    StringView short_name, StringView explanation,
    Function<void()> callback)
{
    auto id = flag_callbacks.count;
    TRY(array_push(&flags, (Flag){
        long_name, short_name, explanation 
    }));
    TRY(array_push(&flag_callbacks, callback));
    TRY(long_flag_ids.append(long_name, id));
    TRY(short_flag_ids.append(short_name, id));

    return {};
}

ErrorOr<void> ArgumentParser::add_option(StringView long_name,
    StringView short_name, StringView placeholder,
    StringView explanation,
    Function<void(c_string)> callback)
{
    auto id = option_callbacks.count;
    TRY(array_push(&options, (Option){
        long_name,
        short_name,
        explanation,
        placeholder,
    }));
    TRY(array_push(&option_callbacks, callback));

    TRY(long_option_ids.append(long_name, id));
    TRY(short_option_ids.append(short_name, id));

    return {};
}

ErrorOr<void> ArgumentParser::add_positional_argument(StringView placeholder,
    Function<void(c_string)> callback)
{
    TRY(array_push(&positional_placeholders, placeholder));
    TRY(array_push(&positional_callbacks, callback));

    return {};
}

ArgumentParserResult ArgumentParser::run(int argc,
    c_string argv[]) const
{
    m_argv = argv;
    auto err_out = StringBuffer();

    c_string program_name = argv[0];
    auto program_name_view
        = StringView::from_c_string(program_name);
    usize used_positionals = 0;
    for (int i = 1; i < argc; i++) {
        auto argument = StringView::from_c_string(argv[i]);
        if (auto id = short_flag_ids.find(argument); id) {
            flag_callbacks.items[id->raw()]();
            continue;
        }

        if (auto id = long_flag_ids.find(argument); id) {
            flag_callbacks.items[id->raw()]();
            continue;
        }

        if (auto id = short_option_ids.find(argument); id) {
            if (i + 1 >= argc) {
                TRY(err_out.writeln(
                    "No argument provided for argument \""sv,
                    argument, "\""sv));
                TRY(err_out.writeln("\nSee help for more info ("sv,
                    program_name_view, " --help)"sv));
                return ArgumentParserError { move(err_out) };
            }
            c_string value = argv[++i];
            option_callbacks.items[id->raw()](value);
            continue;
        }

        if (auto id = long_option_ids.find(argument); id) {
            if (i + 1 >= argc) {
                TRY(err_out.writeln("No argument provided for \""sv,
                    argument, "\"\n"sv));
                TRY(err_out.writeln("See help for more info ("sv,
                    program_name_view, " --help)"sv));
                return ArgumentParserError { move(err_out) };
            }
            c_string value = argv[++i];
            option_callbacks.items[id->raw()](value);
            continue;
        }

        if (used_positionals < positional_callbacks.count) {
            auto id = used_positionals++;
            positional_callbacks.items[id](argument.data());
            continue;
        }

        TRY(err_out.writeln("Unrecognized argument: \""sv, argument,
            "\""sv));
        TRY(err_out.writeln("\nSee help for more info ("sv,
            program_name_view, " --help)"sv));

        return ArgumentParserError { move(err_out) };
    }

    if (used_positionals != positional_placeholders.count) {
        if (positional_placeholders.count - used_positionals
            == 1) {
            auto placeholder
                = positional_placeholders.items[used_positionals];

            TRY(err_out.writeln("Missing positional argument: "sv,
                placeholder));
        } else {
            TRY(err_out.writeln("Missing positional arguments: "sv));
            for (usize i = used_positionals;
                 i < positional_placeholders.count; i++) {
                auto placeholder = positional_placeholders.items[i];
                TRY(err_out.writeln("\t"sv, placeholder));
            }
        }

        TRY(err_out.writeln("\nSee help for more info ("sv,
            program_name_view, " --help)"sv));

        return ArgumentParserError { move(err_out) };
    }

    return {};
}

void ArgumentParser::print_usage_and_exit(int exit_code) const
{
    auto log = file_logger_init(exit_code == 0 ? stdout : stderr);
    log->format("USAGE: %s ", m_argv[0] ?: getenv("_"));
    if (options.count == 0) {
        log->format("[flags] ");
    } else {
        log->format("[flags|options] ");
    }
    for (auto placeholder : array_iter(positional_placeholders)) {
        log->format("%.*s", placeholder.size(), placeholder.data());
    }
    log->format("\n\n");
    log->format("FLAGS:\n");
    u32 longest_short_flag = 0;
    u32 longest_long_flag = 0;
    for (u32 i = 0; i < flags.count; i++) {
        auto flag = flags.items[i];
        if (flag.short_name.size() > longest_short_flag)
            longest_short_flag = flag.short_name.size();
        if (flag.long_name.size() > longest_long_flag)
            longest_long_flag = flag.long_name.size();
    }

    for (u32 i = 0; i < flags.count; i++) {
        auto flag = flags.items[i];
        log->format("        %.*s%*c%.*s%*c",
            flag.short_name.size(), flag.short_name.data(),
            (longest_short_flag - flag.short_name.size() + 1), ' ',
            flag.long_name.size(), flag.long_name.data(),
            (longest_long_flag - flag.long_name.size() + 1), ' '
        );
        log->format("%.*s\n", flag.explanation.size(), flag.explanation.data());
    }
    log->format("\n");
    if (options.count != 0) {
        log->format("OPTIONS:\n");
        for (auto option : array_iter(options)) {
            // auto pad = option.short_name.size() == 2 ? " "sv : ""sv;
            // auto bytes = MUST(out.write("        "sv, option.short_name,
            //     ", "sv, pad, option.long_name, "  <"sv,
            //     option.placeholder, "> "sv));
            // for (; bytes < 40; bytes++)
            //     out.write(" "sv).ignore();
            log->format("        %.*s, %.*s <%.*s> %.*s\n",
                option.short_name.size(), option.short_name.data(),
                option.long_name.size(), option.long_name.data(),
                option.placeholder.size(), option.placeholder.data(),
                option.explanation.size(), option.explanation.data()
            );
        }
        log->format("\n");
    }
    exit(exit_code);
}


}
