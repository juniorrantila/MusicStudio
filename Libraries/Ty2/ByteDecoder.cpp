#include "./ByteDecoder.h"

#include "./StringView.h"
#include "./Bits.h"
#include "Bytes.h"

#include <string.h>

void ByteDecoder::skip(u64 count) { return byte_skip(this, count); }
C_API void byte_skip(ByteDecoder* decoder, u64 count)
{
    decoder->cursor += count;
}

ByteFound ByteDecoder::peek_string(u64 count, StringView2* out) const { return byte_peek_string(this, count, out); }
C_API ByteFound byte_peek_string(ByteDecoder const* decoder, u64 count, StringView2* out)
{
    Bytes bytes;
    if (!byte_peek_bytes(decoder, count, &bytes).found)
        return byte_not_found();
    if (out) *out = sv_from_parts((char const*)bytes.items, bytes.count);
    return byte_found();
}

ByteFound ByteDecoder::peek_bytes(u64 count, Bytes* out) const { return byte_peek_bytes(this, count, out); }
C_API ByteFound byte_peek_bytes(ByteDecoder const* decoder, u64 count, Bytes* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, count);
    if (bytes.count != count)
        return byte_not_found();
    if (out) *out = bytes;
    return byte_found();
}

ByteFound ByteDecoder::peek_u8(u8* out) const { return byte_peek_u8(this, out); }
C_API ByteFound byte_peek_u8(ByteDecoder const* decoder, u8* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u8));
    if (bytes.count != sizeof(u8))
        return byte_not_found();
    *out = bytes.items[0];
    return byte_found();
}

ByteFound ByteDecoder::peek_u16le(u16* out) const { return byte_peek_u16le(this, out); }
C_API ByteFound byte_peek_u16le(ByteDecoder const* decoder, u16* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u16));
    if (bytes.count != sizeof(u16))
        return byte_not_found();
    *out = ty_device_endian_from_u16le(*(u16*)bytes.items);
    return byte_found();
}

ByteFound ByteDecoder::peek_u32le(u32* out) const { return byte_peek_u32le(this, out); }
C_API ByteFound byte_peek_u32le(ByteDecoder const* decoder, u32* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u32));
    if (bytes.count != sizeof(u32))
        return byte_not_found();
    *out = ty_device_endian_from_u16le(*(u32*)bytes.items);
    return byte_found();
}

ByteFound ByteDecoder::peek_u64le(u64* out) const { return byte_peek_u64le(this, out); }
C_API ByteFound byte_peek_u64le(ByteDecoder const* decoder, u64* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u64));
    if (bytes.count != sizeof(u64))
        return byte_not_found();
    *out = ty_device_endian_from_u16le(*(u64*)bytes.items);
    return byte_found();
}

ByteFound ByteDecoder::peek_u16be(u16* out) const { return byte_peek_u16be(this, out); }
C_API ByteFound byte_peek_u16be(ByteDecoder const* decoder, u16* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u16));
    if (bytes.count != sizeof(u16))
        return byte_not_found();
    *out = ty_device_endian_from_u16be(*(u16*)bytes.items);
    return byte_found();
}

ByteFound ByteDecoder::peek_u32be(u32* out) const { return byte_peek_u32be(this, out); }
C_API ByteFound byte_peek_u32be(ByteDecoder const* decoder, u32* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u32));
    if (bytes.count != sizeof(u32))
        return byte_not_found();
    *out = ty_device_endian_from_u16be(*(u32*)bytes.items);
    return byte_found();
}

ByteFound ByteDecoder::peek_u64be(u64* out) const { return byte_peek_u64be(this, out); }
C_API ByteFound byte_peek_u64be(ByteDecoder const* decoder, u64* out)
{
    Bytes bytes = decoder->bytes.slice(decoder->cursor, sizeof(u64));
    if (bytes.count != sizeof(u64))
        return byte_not_found();
    *out = ty_device_endian_from_u16be(*(u64*)bytes.items);
    return byte_found();
}

ByteExpect ByteDecoder::expect(StringView2 value) { return byte_expect_string(this, value); }
C_API ByteExpect byte_expect_string(ByteDecoder* decoder, StringView2 value)
{
    return byte_expect_bytes(decoder, bytes((u8*)value.items, value.count));
}

ByteExpect ByteDecoder::expect(Bytes value) { return byte_expect_bytes(this, value); }
C_API ByteExpect byte_expect_bytes(ByteDecoder* decoder, Bytes value)
{
    Bytes bytes;
    if (!byte_peek_bytes(decoder, value.count, &bytes).found)
        return byte_error();
    if (!bytes_equal(bytes, value))
        return byte_error();
    byte_skip(decoder, value.count);
    return byte_ok();
}

ByteExpect ByteDecoder::expect(u8 value) { return byte_expect_u8(this, value); }
C_API ByteExpect byte_expect_u8(ByteDecoder* decoder, u8 value)
{
    u8 result;
    if (!byte_peek_u8(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u8));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u16le(u16 value) { return byte_expect_u16le(this, value); }
C_API ByteExpect byte_expect_u16le(ByteDecoder* decoder, u16 value)
{
    u16 result;
    if (!byte_peek_u16le(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u16));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u32le(u32 value) { return byte_expect_u32le(this, value); }
C_API ByteExpect byte_expect_u32le(ByteDecoder* decoder, u32 value)
{
    u32 result;
    if (!byte_peek_u32le(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u32));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u64le(u64 value) { return byte_expect_u64le(this, value); }
C_API ByteExpect byte_expect_u64le(ByteDecoder* decoder, u64 value)
{
    u64 result;
    if (!byte_peek_u64le(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u64));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u16be(u16 value) { return byte_expect_u16be(this, value); }
C_API ByteExpect byte_expect_u16be(ByteDecoder* decoder, u16 value)
{
    u16 result;
    if (!byte_peek_u16be(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u16));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u32be(u32 value) { return byte_expect_u32be(this, value); }
C_API ByteExpect byte_expect_u32be(ByteDecoder* decoder, u32 value)
{
    u32 result;
    if (!byte_peek_u32be(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u32));
    return byte_ok();
}

ByteExpect ByteDecoder::expect_u64be(u64 value) { return byte_expect_u64be(this, value); }
C_API ByteExpect byte_expect_u64be(ByteDecoder* decoder, u64 value)
{
    u64 result;
    if (!byte_peek_u64be(decoder, &result).found)
        return byte_error();
    if (result != value)
        return byte_error();
    byte_skip(decoder, sizeof(u64));
    return byte_ok();
}

ByteFound ByteDecoder::parse_string(u64 count, StringView2* out) { return byte_parse_string(this, count, out); }
C_API ByteFound byte_parse_string(ByteDecoder* decoder, u64 count, StringView2* out)
{
    if (!byte_peek_string(decoder, count, out).found)
        return byte_not_found();
    byte_skip(decoder, count);
    return byte_found();
}

ByteFound ByteDecoder::parse_bytes(u64 count, Bytes* out) { return byte_parse_bytes(this, count, out); }
C_API ByteFound byte_parse_bytes(ByteDecoder* decoder, u64 count, Bytes* out)
{
    if (!byte_peek_bytes(decoder, count, out).found)
        return byte_not_found();
    byte_skip(decoder, count);
    return byte_found();
}

ByteFound ByteDecoder::parse_u8(u8* out) { return byte_parse_u8(this, out); }
C_API ByteFound byte_parse_u8(ByteDecoder* decoder, u8* out)
{
    if (!byte_peek_u8(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u16le(u16* out) { return byte_parse_u16le(this, out); }
C_API ByteFound byte_parse_u16le(ByteDecoder* decoder, u16* out)
{
    if (!byte_peek_u16le(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u32le(u32* out) { return byte_parse_u32le(this, out); }
C_API ByteFound byte_parse_u32le(ByteDecoder* decoder, u32* out)
{
    if (!byte_peek_u32le(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u64le(u64* out) { return byte_parse_u64le(this, out); }
C_API ByteFound byte_parse_u64le(ByteDecoder* decoder, u64* out)
{
    if (!byte_peek_u64le(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u16be(u16* out) { return byte_parse_u16be(this, out); }
C_API ByteFound byte_parse_u16be(ByteDecoder* decoder, u16* out)
{
    if (!byte_peek_u16be(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u32be(u32* out) { return byte_parse_u32be(this, out); }
C_API ByteFound byte_parse_u32be(ByteDecoder* decoder, u32* out)
{
    if (!byte_peek_u32be(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}

ByteFound ByteDecoder::parse_u64be(u64* out) { return byte_parse_u64be(this, out); }
C_API ByteFound byte_parse_u64be(ByteDecoder* decoder, u64* out)
{
    if (!byte_peek_u64be(decoder, out).found)
        return byte_not_found();
    byte_skip(decoder, sizeof(*out));
    return byte_found();
}
