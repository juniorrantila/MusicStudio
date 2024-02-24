#pragma once
#include "StringView.h"

namespace std {

struct source_location {
    struct __impl {
        char const* _M_file_name;
        char const* _M_function_name;
        unsigned _M_line;
        unsigned _M_column;
    };

    static consteval source_location current(
        __impl const* __p = __builtin_source_location())
    {
        source_location __ret;
        __ret._M_impl = static_cast<__impl const*>(__p);
        return __ret;
    }

    consteval char const* function_name() const
    {
        return _M_impl ? _M_impl->_M_function_name : "";
    }

    __impl const* _M_impl { nullptr };
};

}

namespace Ty {

struct Type {
    template <typename T>
    consteval static StringView name_of()
    {
        constexpr auto name = StringView(
            std::source_location::current().function_name());
        constexpr auto start
            = "static Ty::StringView Ty::Type::name_of() [T = "sv
                  .size;
        constexpr auto end = name.size - "]"sv.size;
        return name.part(start, end);
    }
    template <typename T>
    struct AddName {
        static consteval StringView type_name()
        {
            return Type::name_of<T>();
        }
    };

    template <typename T>
    consteval static void const* hash_of()
    {
        return std::source_location::current().function_name();
    }
    template <typename T>
    struct AddHash {
        static consteval void const* type_hash()
        {
            return Type::hash_of<T>();
        }
    };
};

}
