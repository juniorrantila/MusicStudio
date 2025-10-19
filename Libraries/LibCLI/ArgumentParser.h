#pragma once
#include <Basic/Function.h>

#include <LibTy/Base.h>
#include <LibTy/Move.h>
#include <LibTy/SmallMap.h>
#include <LibTy/SmallVector.h>
#include <LibTy/StringBuffer.h>
#include <LibTy/StringView.h>
#include <LibTy/Try.h>

#include "./ArgumentParserError.h"

namespace CLI {

struct ArgumentParser {
    ArgumentParser();

    ErrorOr<void> add_flag(StringView long_name,
        StringView short_name, StringView explanation,
        Function<void()> callback);

    ErrorOr<void> add_option(StringView long_name,
        StringView short_name, StringView placeholder,
        StringView explanation,
        Function<void(c_string)> callback);

    ErrorOr<void> add_positional_argument(StringView placeholder,
        Function<void(c_string)> callback);

    ArgumentParserResult run(int argc, c_string argv[]) const;

    void print_usage_and_exit(int exit_code = 0) const;

private:
    mutable c_string* m_argv { nullptr };
    struct Flag {
        StringView long_name;
        StringView short_name;
        StringView explanation;
    };
    struct Flags {
        Flag* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };
    Flags flags {};

    struct Option {
        StringView long_name;
        StringView short_name;
        StringView explanation;
        StringView placeholder;
    };
    struct Options {
        Option* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };
    Options options {};

    struct StringArray {
        StringView* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };
    SmallMap<StringView, u32> short_flag_ids {};
    SmallMap<StringView, u32> long_flag_ids {};

    SmallMap<StringView, u32> short_option_ids {};
    SmallMap<StringView, u32> long_option_ids {};

    struct FlagCallbacks {
        Function<void()>* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };

    struct OptionCallbacks {
        Function<void(c_string)>* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };

    struct PositionalCallbacks {
        Function<void(c_string)>* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };

    FlagCallbacks flag_callbacks {};
    OptionCallbacks option_callbacks {};

    struct Placeholders {
        StringView* items = nullptr;
        u32 count = 0;
        u32 capacity = 0;
    };
    Placeholders positional_placeholders {};
    PositionalCallbacks positional_callbacks {};
};
}
