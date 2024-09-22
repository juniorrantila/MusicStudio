#pragma once
#include <Ty/Base.h>
#include <Ty/Move.h>
#include <Ty/SmallCapture.h>
#include <Ty/SmallMap.h>
#include <Ty/SmallVector.h>
#include <Ty/StringBuffer.h>
#include <Ty/StringView.h>
#include <Ty/Try.h>
#include "./ArgumentParserError.h"

namespace CLI {

struct ArgumentParser {
    ArgumentParser();

    ErrorOr<void> add_flag(StringView long_name,
        StringView short_name, StringView explanation,
        SmallCapture<void()>&& callback);

    ErrorOr<void> add_option(StringView long_name,
        StringView short_name, StringView placeholder,
        StringView explanation,
        SmallCapture<void(c_string)>&& callback);

    ErrorOr<void> add_positional_argument(StringView placeholder,
        SmallCapture<void(c_string)>&& callback);

    ArgumentParserResult run(int argc, c_string argv[]) const;

    void print_usage_and_exit(c_string program_name,
        int exit_code = 0) const;

private:
    mutable c_string* m_argv { nullptr };
    struct Flag {
        StringView long_name;
        StringView short_name;
        StringView explanation;
    };
    SmallVector<Flag> flags {};

    struct Option {
        StringView long_name;
        StringView short_name;
        StringView explanation;
        StringView placeholder;
    };
    SmallVector<Option> options {};

    SmallMap<StringView, u32> short_flag_ids {};
    SmallMap<StringView, u32> long_flag_ids {};

    SmallMap<StringView, u32> short_option_ids {};
    SmallMap<StringView, u32> long_option_ids {};

    SmallVector<SmallCapture<void()>> flag_callbacks {};
    SmallVector<SmallCapture<void(c_string)>> option_callbacks {};

    SmallVector<StringView> positional_placeholders {};
    SmallVector<SmallCapture<void(c_string)>>
        positional_callbacks {};
};
}
