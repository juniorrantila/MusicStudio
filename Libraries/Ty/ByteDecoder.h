#pragma once
#include "./Forward.h"

#include "./Bytes.h"

namespace Ty {

struct ByteDecoder {
    Bytes bytes {};
    usize cursor { 0 };

    Optional<Bytes> peek(usize count) const;
    Optional<StringView> peeks(usize count) const;
    void skip(usize bytes) { cursor += bytes; }

    Optional<u8> peek_u8() const;
    Optional<u16> peek_u16le() const;
    Optional<u32> peek_u32le() const;
    Optional<u64> peek_u64le() const;
    Optional<u16> peek_u16be() const;
    Optional<u32> peek_u32be() const;
    Optional<u64> peek_u64be() const;

    [[nodiscard]] bool expect(StringView value);
    [[nodiscard]] bool expect(Bytes value);
    [[nodiscard]] bool expect_u8(u8);
    [[nodiscard]] bool expect_u16le(u16);
    [[nodiscard]] bool expect_u32le(u32);
    [[nodiscard]] bool expect_u64le(u64);
    [[nodiscard]] bool expect_u16be(u16);
    [[nodiscard]] bool expect_u32be(u32);
    [[nodiscard]] bool expect_u64be(u64);

    Optional<u8> parse_u8();

    Optional<u16> parse_u16le();
    Optional<u32> parse_u32le();
    Optional<u64> parse_u64le();

    Optional<u16> parse_u16be();
    Optional<u32> parse_u32be();
    Optional<u64> parse_u64be();
    Optional<StringView> parse_string(usize count);
    Optional<Bytes> parse_bytes(usize count);
};


}
