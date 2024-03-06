#include "ArgumentParser.h"
#include <Core/File.h>
#include <Ty/Move.h>

namespace CLI {

ArgumentParser::ArgumentParser()
{
    MUST(add_flag("--help"sv, "-h"sv, "show this message"sv, [this] {
        this->print_usage_and_exit(nullptr);
    }));
}

ErrorOr<void> ArgumentParser::add_flag(StringView long_name,
    StringView short_name, StringView explanation,
    SmallCapture<void()>&& callback)
{
    auto id = flag_callbacks.size();
    TRY(flags.append({ long_name, short_name, explanation }));
    TRY(flag_callbacks.append(move(callback)));
    TRY(long_flag_ids.append(long_name, id));
    TRY(short_flag_ids.append(short_name, id));

    return {};
}

ErrorOr<void> ArgumentParser::add_option(StringView long_name,
    StringView short_name, StringView placeholder,
    StringView explanation,
    SmallCapture<void(c_string)>&& callback)
{
    auto id = option_callbacks.size();
    TRY(options.append({
        long_name,
        short_name,
        explanation,
        placeholder,
    }));
    TRY(option_callbacks.append(move(callback)));

    TRY(long_option_ids.append(long_name, id));
    TRY(short_option_ids.append(short_name, id));

    return {};
}

ErrorOr<void> ArgumentParser::add_positional_argument(StringView placeholder,
    SmallCapture<void(c_string)>&& callback)
{
    TRY(positional_placeholders.append(placeholder));
    TRY(positional_callbacks.append(move(callback)));

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
            flag_callbacks[id->raw()]();
            continue;
        }

        if (auto id = long_flag_ids.find(argument); id) {
            flag_callbacks[id->raw()]();
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
            option_callbacks[id->raw()](value);
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
            option_callbacks[id->raw()](value);
            continue;
        }

        if (used_positionals < positional_callbacks.size()) {
            auto id = used_positionals++;
            positional_callbacks[id](argument.data());
            continue;
        }

        TRY(err_out.writeln("Unrecognized argument: \""sv, argument,
            "\""sv));
        TRY(err_out.writeln("\nSee help for more info ("sv,
            program_name_view, " --help)"sv));

        return ArgumentParserError { move(err_out) };
    }

    if (used_positionals != positional_placeholders.size()) {
        if (positional_placeholders.size() - used_positionals
            == 1) {
            auto placeholder
                = positional_placeholders[used_positionals];

            TRY(err_out.writeln("Missing positional argument: "sv,
                placeholder));
        }
        TRY(err_out.writeln("Missing positional arguments: "sv));
        for (usize i = used_positionals;
             i < positional_placeholders.size(); i++) {
            auto placeholder = positional_placeholders[i];
            TRY(err_out.writeln("\t"sv, placeholder));
        }

        TRY(err_out.writeln("\nSee help for more info ("sv,
            program_name_view, " --help)"sv));

        return ArgumentParserError { move(err_out) };
    }

    return {};
}

void ArgumentParser::print_usage_and_exit(c_string program_name,
    int exit_code) const
{
    auto& out = exit_code != 0 ? Core::File::stderr()
                               : Core::File::stdout();
    auto program_name_view
        = StringView::from_c_string(program_name ?: m_argv[0]);
    out.write("USAGE: "sv, program_name_view, " "sv).ignore();
    if (options.is_empty()) {
        out.write("[flags] "sv)
            .ignore();
    } else {
        out.write("[flags|options] "sv)
            .ignore();
    }
    for (auto positional_argument : positional_placeholders)
        out.write(positional_argument, " "sv).ignore();
    out.write("\n\n"sv).ignore();
    out.writeln("FLAGS:"sv).ignore();
    for (auto flag : flags) {
        auto pad = flag.short_name.size() == 2 ? " "sv : ""sv;
        auto bytes = MUST(out.write("        "sv, flag.short_name,
            ", "sv, pad, flag.long_name));
        for (; bytes < 40; bytes++)
            out.write(" "sv).ignore();
        out.writeln(flag.explanation).ignore();
    }
    out.writeln().ignore();
    if (!options.is_empty()) {
        out.writeln("OPTIONS:"sv).ignore();
        for (auto option : options) {
            auto pad = option.short_name.size() == 2 ? " "sv : ""sv;
            auto bytes = MUST(out.write("        "sv, option.short_name,
                ", "sv, pad, option.long_name, "  <"sv,
                option.placeholder, "> "sv));
            for (; bytes < 40; bytes++)
                out.write(" "sv).ignore();
            out.writeln(option.explanation).ignore();
        }
        out.writeln().ignore();
    }
    System::exit(exit_code);
}


}
