#pragma once
#include "./Forward.h"
#include "./Types.h"

#include "./Bytes.h"

typedef struct [[nodiscard]] { bool found; } ByteFound;
C_INLINE ByteFound byte_found(void) { return (ByteFound){true}; }
C_INLINE ByteFound byte_not_found(void) { return (ByteFound){false}; }

typedef struct [[nodiscard]] { bool ok; } ByteExpect;
C_INLINE ByteExpect byte_ok(void) { return (ByteExpect){true}; }
C_INLINE ByteExpect byte_error(void) { return (ByteExpect){false}; }

typedef struct ByteDecoder {
    Bytes bytes;
    u64 cursor;

#if __cplusplus

    void skip(u64 count);

    ByteFound peek_bytes(u64 count, Bytes*) const;
    ByteFound peek_string(u64 count, StringSlice*) const;
    ByteFound peek_u8(u8*) const;

    ByteFound peek_u16le(u16*) const;
    ByteFound peek_u32le(u32*) const;
    ByteFound peek_u64le(u64*) const;

    ByteFound peek_u16be(u16*) const;
    ByteFound peek_u32be(u32*) const;
    ByteFound peek_u64be(u64*) const;

    ByteFound peek_ileb128(i64*) const;
    ByteFound peek_uleb128(u64*) const;

    ByteExpect expect(StringSlice);
    ByteExpect expect(Bytes);
    ByteExpect expect(u8);

    ByteExpect expect_u16le(u16);
    ByteExpect expect_u32le(u32);
    ByteExpect expect_u64le(u64);

    ByteExpect expect_u16be(u16);
    ByteExpect expect_u32be(u32);
    ByteExpect expect_u64be(u64);

    ByteExpect expect_ileb128(i64);
    ByteExpect expect_uleb128(u64);

    ByteFound parse_u8(u8*);
    ByteFound parse_string(u64 count, StringSlice*);
    ByteFound parse_bytes(u64 count, Bytes*);

    ByteFound parse_u16le(u16*);
    ByteFound parse_u32le(u32*);
    ByteFound parse_u64le(u64*);

    ByteFound parse_u16be(u16*);
    ByteFound parse_u32be(u32*);
    ByteFound parse_u64be(u64*);

    ByteFound parse_ileb128(i64*);
    ByteFound parse_uleb128(u64*);

#endif
} ByteDecoder;

C_INLINE ByteDecoder byte_decoder(Bytes bytes)
{
    return (ByteDecoder){
        .bytes = bytes,
        .cursor = 0,
    };
}

C_API void byte_skip(ByteDecoder*, u64 count);

C_API ByteFound byte_peek_string(ByteDecoder const*, u64 count, StringSlice*);
C_API ByteFound byte_peek_bytes(ByteDecoder const*, u64 count, Bytes*);
C_API ByteFound byte_peek_u8(ByteDecoder const*, u8*);

C_API ByteFound byte_peek_u16le(ByteDecoder const*, u16*);
C_API ByteFound byte_peek_u32le(ByteDecoder const*, u32*);
C_API ByteFound byte_peek_u64le(ByteDecoder const*, u64*);

C_API ByteFound byte_peek_u16be(ByteDecoder const*, u16*);
C_API ByteFound byte_peek_u32be(ByteDecoder const*, u32*);
C_API ByteFound byte_peek_u64be(ByteDecoder const*, u64*);

C_API ByteFound byte_peek_ileb128(ByteDecoder const*, i64*);
C_API ByteFound byte_peek_uleb128(ByteDecoder const*, u64*);

C_API ByteExpect byte_expect_string(ByteDecoder*, StringSlice);
C_API ByteExpect byte_expect_bytes(ByteDecoder*, Bytes value);
C_API ByteExpect byte_expect_u8(ByteDecoder*, u8);

C_API ByteExpect byte_expect_u16le(ByteDecoder*, u16);
C_API ByteExpect byte_expect_u32le(ByteDecoder*, u32);
C_API ByteExpect byte_expect_u64le(ByteDecoder*, u64);
C_API ByteExpect byte_expect_u16be(ByteDecoder*, u16);
C_API ByteExpect byte_expect_u32be(ByteDecoder*, u32);
C_API ByteExpect byte_expect_u64be(ByteDecoder*, u64);

C_API ByteExpect byte_expect_ileb128(ByteDecoder*, i64);
C_API ByteExpect byte_expect_uleb128(ByteDecoder*, u64);

C_API ByteFound byte_parse_string(ByteDecoder*, u64 count, StringSlice*);
C_API ByteFound byte_parse_bytes(ByteDecoder*, u64 count, Bytes*);
C_API ByteFound byte_parse_u8(ByteDecoder*, u8*);

C_API ByteFound byte_parse_u16le(ByteDecoder*, u16*);
C_API ByteFound byte_parse_u32le(ByteDecoder*, u32*);
C_API ByteFound byte_parse_u64le(ByteDecoder*, u64*);

C_API ByteFound byte_parse_u16be(ByteDecoder*, u16*);
C_API ByteFound byte_parse_u32be(ByteDecoder*, u32*);
C_API ByteFound byte_parse_u64be(ByteDecoder*, u64*);

C_API ByteFound byte_parse_ileb128(ByteDecoder*, i64*);
C_API ByteFound byte_parse_uleb128(ByteDecoder*, u64*);
