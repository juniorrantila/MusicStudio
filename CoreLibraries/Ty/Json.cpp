#include "./Json.h"

#include "./Assert.h"
#include "./Parse.h"
#include "./View.h"

namespace {

struct Token {
    enum Type : u8 {
        Number,
        String,

        OpenCurly,
        CloseCurly,
        OpenBracket,
        CloseBracket,

        Comma,
        Colon,

        Null,

        True,
        False,
    };

    u32 start;
    u32 size;
    Type type;
};

constexpr ErrorOr<Vector<Token>> lex(StringView);
constexpr ErrorOr<JsonValue> parse(StringView, View<Token>,
    Vector<JsonObject>&, Vector<JsonArray>&);

}

namespace Ty {

constexpr Json::Json(JsonValue root, JsonObjects&& objects,
    JsonArrays&& arrays)
    : m_root(root)
    , m_objects(move(objects))
    , m_arrays(move(arrays))
{
}

ErrorOr<Json> Json::create_from(StringView source)
{
    auto tokens = TRY(lex(source));
    auto objects = TRY(JsonObjects::create());
    auto arrays = TRY(JsonArrays::create());
    return Json {
        TRY(parse(source, tokens.view(), objects, arrays)),
        move(objects),
        move(arrays),
    };
}

ErrorOr<StringBuffer> JsonValue::serialize(Json const& json) const
{
    switch (type()) {
    case JsonValue::Type::Bool:
        return TRY(Json::serialize(unsafe_as_bool()));
    case JsonValue::Type::Null:
        return StringBuffer::create_fill("null"sv);
    case JsonValue::Type::Number:
        return TRY(Json::serialize(unsafe_as_number()));
    case JsonValue::Type::String:
        return TRY(Json::serialize(unsafe_as_string()));
    case JsonValue::Type::Array: {
        auto buffer = TRY(StringBuffer::create());
        TRY(buffer.write("["sv));
        auto& array = json[unsafe_as_array()];
        for (usize i = 0; i < array.size(); i++) {
            auto value = array[i];
            TRY(buffer.write(TRY(value.serialize(json))));
            if (i == array.size() - 1)
                continue;
            TRY(buffer.write(","sv));
        }
        TRY(buffer.write("]"sv));
        return buffer;
    }
    case JsonValue::Type::Object: {
        auto buffer = TRY(StringBuffer::create());
        TRY(buffer.write("{"sv));
        auto& object = json[unsafe_as_object()];
        auto keys = object.keys();
        for (usize i = 0; i < keys.size(); i++) {
            auto key = keys[i];
            auto value = *object.fetch(key);
            TRY(buffer.write(TRY(Json::serialize(key))));
            TRY(buffer.write(":"sv));
            TRY(buffer.write(TRY(value.serialize(json))));
            if (i == keys.size() - 1)
                continue;
            TRY(buffer.write(","sv));
        }
        TRY(buffer.write("}"sv));
        return buffer;
    };
    }
}

ErrorOr<StringBuffer> Json::serialize(bool value)
{
    return StringBuffer::create_fill(value);
}

ErrorOr<StringBuffer> Json::serialize(JsonNumber value)
{
    if (auto int_value = value.as_isize()) {
        return StringBuffer::create_fill(*int_value);
    }
    return StringBuffer::create_fill(value.as_f64());
}

ErrorOr<StringBuffer> Json::serialize(StringView value)
{
    return StringBuffer::create_fill("\""sv, value, "\""sv);
}

ErrorOr<StringBuffer> Json::serialize() const
{
    return root().serialize(*this);
}

}

namespace {

// FIXME: Escape strings properly.
constexpr ErrorOr<Token> lex_string(StringView source, u32 start)
{
    auto end = start;
    while (end++ < source.size()) {
        if (source[end] == '"')
            break;
    }
    if (source[end] != '"')
        return Error::from_string_literal("no end quote");
    return Token {
        start,
        end - start + 1,
        Token::String,
    };
}

constexpr bool is_number(char character)
{
    switch (character) {
    case '0' ... '9': return true;
    default: return false;
    }
};

constexpr ErrorOr<Token> lex_number(StringView source, u32 start)
{
    auto end = start;
    while (end++ < source.size()) {
        if (is_number(source[end]))
            continue;
        if (source[end] == '.')
            continue;
        break;
    }
    if (!is_number(source[end - 1]))
        return Error::from_string_literal("invalid number");
    return Token {
        start,
        end - start,
        Token::Number,
    };
}

constexpr ErrorOr<Token> lex_null(StringView source, u32 start)
{
    if (source.sub_view(start, "null"sv.size()) != "null"sv)
        return Error::from_string_literal("invalid null");
    return Token {
        start,
        "null"sv.size(),
        Token::Null,
    };
}

constexpr ErrorOr<Token> lex_true(StringView source, u32 start)
{
    if (source.sub_view(start, "true"sv.size()) != "true"sv)
        return Error::from_string_literal("invalid true");
    return Token {
        start,
        "true"sv.size(),
        Token::True,
    };
}

constexpr ErrorOr<Token> lex_false(StringView source, u32 start)
{
    if (source.sub_view(start, "false"sv.size()) != "false"sv)
        return Error::from_string_literal("invalid false");
    return Token {
        start,
        "false"sv.size(),
        Token::False,
    };
}

constexpr ErrorOr<Token> lex_single_token(StringView source,
    u32 start)
{
    switch (source[start]) {
    case '"': return TRY(lex_string(source, start));
    case '0' ... '9': return TRY(lex_number(source, start));
    case ',': {
        return Token {
            start,
            ","sv.size(),
            Token::Comma,
        };
    }
    case ':': {
        return Token {
            start,
            ":"sv.size(),
            Token::Colon,
        };
    }
    case '{': {
        return Token {
            start,
            "{"sv.size(),
            Token::OpenCurly,
        };
    }
    case '}': {
        return Token {
            start,
            "}"sv.size(),
            Token::CloseCurly,
        };
    }
    case '[': {
        return Token {
            start,
            "["sv.size(),
            Token::OpenBracket,
        };
    }
    case ']': {
        return Token {
            start,
            "]"sv.size(),
            Token::CloseBracket,
        };
    }
    case 'n': return TRY(lex_null(source, start));
    case 't': return TRY(lex_true(source, start));
    case 'f': return TRY(lex_false(source, start));
    default: return Error::from_string_literal("invalid character");
    }
}

constexpr ErrorOr<Vector<Token>> lex(StringView source)
{
    auto tokens = TRY(Vector<Token>::create());

    u32 i = 0;
    while (i < source.size()) {
        if (source[i] == ' ') {
            i++;
            continue;
        }
        if (source[i] == '\n') {
            i++;
            continue;
        }
        if (source[i] == '\t') {
            i++;
            continue;
        }
        if (source[i] == '\r') {
            i++;
            continue;
        }
        auto token = TRY(lex_single_token(source, i));
        TRY(tokens.append(token));
        i += token.size;
    }

    return tokens;
}

struct SingleValue {
    JsonValue value;
    u32 consumed_tokens;
};

ErrorOr<SingleValue> parse_single_value(StringView source,
    View<Token> tokens, JsonObjects& objects, JsonArrays& arrays);

constexpr ErrorOr<SingleValue> parse_number(StringView source,
    Token token)
{
    ASSERT(token.type == Token::Number);
    auto view = source.sub_view(token.start, token.size);
    auto value = TRY(Parse<f64>::from(view).or_throw([] {
        return Error::from_string_literal("invalid number");
    }));
    return SingleValue {
        JsonValue(value),
        1,
    };
}

constexpr ErrorOr<SingleValue> parse_string(StringView source,
    Token token)
{
    ASSERT(token.type == Token::String);
    return SingleValue {
        JsonValue(source.sub_view(token.start + "\""sv.size(),
            token.size - "\"\""sv.size())),
        1,
    };
}

ErrorOr<SingleValue> parse_object(StringView source,
    View<Token> tokens, JsonObjects& objects, JsonArrays& arrays)
{
    auto object = TRY(JsonObject::create());
    ASSERT(tokens[0].type = Token::OpenCurly);
    tokens = View<Token> {
        tokens.data() + 1,
        tokens.size() - 1,
    };
    u32 consumed_tokens = 1;

    // Empty object.
    if (tokens[0].type == Token::CloseCurly) {
        return SingleValue {
            TRY(objects.append(move(object))),
            2,
        };
    }

    while (tokens.size() > 0) {
        ASSERT(tokens.size() > 2);
        ASSERT(tokens[0].type == Token::String);
        auto key = TRY(parse_string(source, tokens[0]));
        ASSERT(tokens[1].type == Token::Colon);
        consumed_tokens += 2;
        tokens = View<Token> {
            tokens.data() + 2,
            tokens.size() - 2,
        };

        auto value = TRY(
            parse_single_value(source, tokens, objects, arrays));
        TRY(object.append(key.value.unsafe_as_string(),
            value.value));

        consumed_tokens += value.consumed_tokens;
        tokens = View<Token> {
            tokens.data() + value.consumed_tokens,
            tokens.size() - value.consumed_tokens,
        };

        if (tokens[0].type == Token::Comma) {
            consumed_tokens++;
            tokens = View<Token> {
                tokens.data() + 1,
                tokens.size() - 1,
            };
            continue;
        }
        break;
    }
    ASSERT(tokens[0].type == Token::CloseCurly);
    return SingleValue {
        TRY(objects.append(move(object))),
        consumed_tokens + 1,
    };
}

ErrorOr<SingleValue> parse_array(StringView source,
    View<Token> tokens, JsonObjects& objects, JsonArrays& arrays)
{
    auto array = TRY(JsonArray::create());

    ASSERT(tokens[0].type == Token::OpenBracket);
    u32 consumed_tokens = 1;
    tokens = View<Token> {
        tokens.data() + 1,
        tokens.size() - 1,
    };

    // Empty array.
    if (tokens[0].type == Token::CloseBracket) {
        return SingleValue {
            TRY(arrays.append(move(array))),
            2,
        };
    }

    while (tokens.size() > 0) {
        auto value = TRY(
            parse_single_value(source, tokens, objects, arrays));
        consumed_tokens += value.consumed_tokens;
        tokens = View<Token> {
            tokens.data() + value.consumed_tokens,
            tokens.size() - value.consumed_tokens,
        };
        TRY(array.append(value.value));
        if (tokens[0].type == Token::Comma) {
            consumed_tokens++;
            tokens = View<Token> {
                tokens.data() + 1,
                tokens.size() - 1,
            };
            continue;
        }
        break;
    }
    ASSERT(tokens[0].type == Token::CloseBracket);
    return SingleValue {
        TRY(arrays.append(move(array))),
        consumed_tokens + 1,
    };
}

constexpr SingleValue parse_null()
{
    return SingleValue { JsonValue(nullptr), 1 };
}

constexpr SingleValue parse_true()
{
    return SingleValue { JsonValue(true), 1 };
}

constexpr SingleValue parse_false()
{
    return SingleValue { JsonValue(false), 1 };
}

ErrorOr<SingleValue> parse_single_value(StringView source,
    View<Token> tokens, JsonObjects& objects, JsonArrays& arrays)
{
    switch (tokens[0].type) {
    case Token::Number: return TRY(parse_number(source, tokens[0]));
    case Token::String: return TRY(parse_string(source, tokens[0]));
    case Token::OpenCurly:
        return TRY(parse_object(source, tokens, objects, arrays));
    case Token::OpenBracket:
        return TRY(parse_array(source, tokens, objects, arrays));
    case Token::Null: return parse_null();
    case Token::True: return parse_true();
    case Token::False: return parse_false();

    case Token::Comma:
        return Error::from_string_literal("unexpected \",\"");
    case Token::Colon:
        return Error::from_string_literal("unexpected \":\"");
    case Token::CloseCurly:
        return Error::from_string_literal("unexpected \"}\"");
    case Token::CloseBracket:
        return Error::from_string_literal("unexpected \"]\"");
    }
}

constexpr ErrorOr<JsonValue> parse(StringView source,
    View<Token> tokens, JsonObjects& objects, JsonArrays& arrays)
{
    auto value
        = TRY(parse_single_value(source, tokens, objects, arrays));
    ASSERT(value.consumed_tokens == tokens.size());
    return value.value;
}

}
