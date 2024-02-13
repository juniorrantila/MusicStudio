#pragma once
#include "Assert.h"
#include "Formatter.h"
#include "Forward.h"
#include "LinearMap.h"
#include "Vector.h"
#include "StringBuffer.h"

namespace Ty {

struct Json;
struct JsonValue;
using JsonObject = LinearMap<StringView, JsonValue>;
using JsonArray = Vector<JsonValue>;

using JsonObjects = Vector<JsonObject>;
using JsonArrays = Vector<JsonArray>;

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
    constexpr double unsafe_as_number() const { return m_number; }
    constexpr Id<JsonObject> unsafe_as_object() const { return m_object; }
    constexpr Id<JsonArray> unsafe_as_array() const { return m_array; }

    constexpr StringView unsafe_as_string() const { return m_string; }

    constexpr ErrorOr<bool> as_bool() const
    {
        ASSERT(m_type == Bool);
        return m_bool;
    }

    constexpr ErrorOr<double> as_number() const
    {
        ASSERT(m_type == Number);
        return m_number;
    }
    constexpr ErrorOr<Id<JsonObject>> as_object() const
    {
        ASSERT(m_type == Object);
        return m_object;
    }
    constexpr ErrorOr<Id<JsonArray>> as_array() const
    {
        ASSERT(m_type == Array);
        return m_array;
    }
    constexpr ErrorOr<StringView> as_string() const
    {
        ASSERT(m_type == String);
        return m_string;
    }

private:
    union {
        Id<JsonArray> m_array;
        Id<JsonObject> m_object;
        StringView m_string;
        bool m_bool;
        double m_number;
    };
    Type m_type;
};

struct Json {
    static ErrorOr<Json> create_from(StringView);

    template <typename T>
        requires requires(T const& value) { T::serialize(value); }
    static ErrorOr<StringBuffer> serialize(T const& value)
    {
        return T::serialize(value);
    }

    static ErrorOr<StringBuffer> serialize(bool value);
    static ErrorOr<StringBuffer> serialize(double value);
    static ErrorOr<StringBuffer> serialize(StringView value);

    constexpr JsonObject const& operator[](Id<JsonObject> id) const { return m_objects[id]; }

    constexpr JsonArray const& operator[](Id<JsonArray> id) const { return m_arrays[id]; }

    JsonValue const& root() const { return m_root; }

private:
    constexpr Json(JsonValue root, JsonObjects&& objects, JsonArrays&& arrays);

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
