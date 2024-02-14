#include "ArgumentParser.h"
#include <Core/File.h>
#include <Ty/Move.h>

namespace CLI {

ErrorOr<void> ArgumentParserError::show() const
{
    switch (m_state) {
    case State::Buffer: {
        TRY(Core::File::stderr().write(m_buffer));
    } break;
    case State::Error: {
        TRY(Core::File::stderr().write(m_error));
    } break;
    case State::Invalid: break;
    }

    return {};
}

ArgumentParserResult ArgumentParser::run(int argc,
    c_string argv[]) const
{
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
        = StringView::from_c_string(program_name);
    out.write("USAGE: "sv, program_name_view, " [flags|options] "sv)
        .ignore();
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
    out.writeln("\nOPTIONS:"sv).ignore();
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
    out.flush().ignore();

    Core::System::exit(exit_code);
}
}
