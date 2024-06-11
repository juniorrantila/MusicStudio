#pragma once
#include "./Forward.h"
#include "./LinearMap.h"
#include "./Vector.h"
#include "./StringBuffer.h"

namespace Ty {

struct Json;
struct JsonValue;
using JsonObject = LinearMap<StringView, JsonValue>;
using JsonArray = Vector<JsonValue>;

using JsonObjects = Vector<JsonObject>;
using JsonArrays = Vector<JsonArray>;

struct JsonNumber {
    constexpr JsonNumber(f64 number)
        : m_number(number)
    {
    }

    constexpr operator f64() const { return as_f64();; }
    constexpr f64 as_f64() const { return m_number; }

    constexpr Optional<isize> as_isize() const
    {
        if (m_number != (f64)(isize)m_number)
            return {};
        return (isize)m_number;
    }

    constexpr Optional<usize> as_usize() const
    {
        if (m_number < 0.0)
            return {};
        if (m_number != (f64)(usize)m_number)
            return {};
        return (usize)m_number;
    }

private:
    f64 m_number { 0.0 };
};

struct JsonValue {
    enum Type : u8 {
        Array,
        Bool,
        Null,
        Number,
        Object,
        String,
    };

    constexpr JsonValue(bool value)
        : m_bool(value)
        , m_type(Bool)
    {
    }

    constexpr JsonValue(nullptr_t)
        : m_type(Null)
    {
    }

    constexpr JsonValue(double value)
        : m_number(value)
        , m_type(Number)
    {
    }

    constexpr JsonValue(Id<JsonArray> value)
        : m_array(value)
        , m_type(Array)
    {
    }

    constexpr JsonValue(Id<JsonObject> value)
        : m_object(value)
        , m_type(Object)
    {
    }

    constexpr JsonValue(StringView value)
        : m_string(value)
        , m_type(String)
    {
    }

    constexpr Type type() const { return m_type; }

    constexpr bool unsafe_as_bool() const { return m_bool; }
    constexpr JsonNumber unsafe_as_number() const { return m_number; }
    constexpr Id<JsonObject> unsafe_as_object() const { return m_object; }
    constexpr Id<JsonArray> unsafe_as_array() const { return m_array; }

    constexpr StringView unsafe_as_string() const { return m_string; }

    constexpr Optional<bool> as_bool() const
    {
        if (m_type != Bool) {
            return {};
        }
        return m_bool;
    }

    constexpr Optional<JsonNumber> as_number() const
    {
        if (m_type != Number) {
            return {};
        }
        return m_number;
    }

    constexpr Optional<Id<JsonObject>> as_object() const
    {
        if (m_type != Object)
            return {};
        return m_object;
    }

    constexpr Optional<Id<JsonArray>> as_array() const
    {
        if (m_type != Array) {
            return {};
        }
        return m_array;
    }

    constexpr Optional<StringView> as_string() const
    {
        if (m_type != String) {
            return {};
        }
        return m_string;
    }

    ErrorOr<StringBuffer> serialize(Json const&) const;

private:
    union {
        Id<JsonArray> m_array;
        Id<JsonObject> m_object;
        StringView m_string;
        bool m_bool;
        JsonNumber m_number;
    };
    Type m_type;
};

struct Json {
    friend JsonValue;
    static ErrorOr<Json> create_from(StringView);

    ErrorOr<StringBuffer> serialize() const;

    constexpr JsonObject const& operator[](Id<JsonObject> id) const { return m_objects[id]; }
    constexpr JsonArray const& operator[](Id<JsonArray> id) const { return m_arrays[id]; }

    JsonValue const& root() const { return m_root; }

private:
    constexpr Json(JsonValue root, JsonObjects&& objects, JsonArrays&& arrays);

    template <typename T>
        requires requires(T const& value) { T::serialize(value); }
    static ErrorOr<StringBuffer> serialize(T const& value)
    {
        return T::serialize(value);
    }

    static ErrorOr<StringBuffer> serialize(bool value);
    static ErrorOr<StringBuffer> serialize(JsonNumber value);
    static ErrorOr<StringBuffer> serialize(StringView value);

    JsonValue m_root;
    JsonObjects m_objects;
    JsonArrays m_arrays;
};

}
using Ty::Json;        // NOLINT
using Ty::JsonArray;   // NOLINT
using Ty::JsonArrays;  // NOLINT
using Ty::JsonObject;  // NOLINT
using Ty::JsonObjects; // NOLINT
using Ty::JsonValue;   // NOLINT

template <>
struct Ty::Formatter<JsonValue::Type> {
    template <typename U>
        requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, JsonValue::Type type)
    {
        auto type_name = [](JsonValue::Type type) {
            switch (type) {
            case JsonValue::Array: return "Array"sv;
            case JsonValue::Bool: return "Bool"sv;
            case JsonValue::Null: return "Null"sv;
            case JsonValue::Number: return "Number"sv;
            case JsonValue::Object: return "Object"sv;
            case JsonValue::String: return "String"sv;
            }
        };
        return TRY(to.write("JsonValue::Type::"sv, type_name(type)));
    }
};
