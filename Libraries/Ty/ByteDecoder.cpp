#include "./ByteDecoder.h"

#include "./StringView.h"
#include "./Endian.h"

namespace Ty {

bool ByteDecoder::expect(StringView value)
{
    if (peeks(value.size()) == value) {
        cursor += value.size();
        return true;
    }
    return false;
}

bool ByteDecoder::expect(Bytes value)
{
    if (peek(value.size()) == value) {
        cursor += value.size();
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u8(u8 value)
{
    if (peek_u8() == value) {
        cursor += sizeof(u8);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u16le(u16 value)
{
    if (peek_u16le() == value) {
        cursor += sizeof(u16);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u32le(u32 value)
{
    if (peek_u32le() == value) {
        cursor += sizeof(u32);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u64le(u64 value)
{
    if (peek_u64le() == value) {
        cursor += sizeof(u64);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u16be(u16 value)
{
    if (peek_u16be() == value) {
        cursor += sizeof(u16);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u32be(u32 value)
{
    if (peek_u32be() == value) {
        cursor += sizeof(u32);
        return true;
    }
    return false;
}

bool ByteDecoder::expect_u64be(u64 value)
{
    if (peek_u64be() == value) {
        cursor += sizeof(u64);
        return true;
    }
    return false;
}

Optional<u8> ByteDecoder::parse_u8()
{
    auto value = peek_u8();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u16> ByteDecoder::parse_u16le()
{
    auto value = peek_u16le();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u32> ByteDecoder::parse_u32le()
{
    auto value = peek_u32le();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u64> ByteDecoder::parse_u64le()
{
    auto value = peek_u64le();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u16> ByteDecoder::parse_u16be()
{
    auto value = peek_u16be();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u32> ByteDecoder::parse_u32be()
{
    auto value = peek_u32be();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<u64> ByteDecoder::parse_u64be()
{
    auto value = peek_u64be();
    if (value.has_value()) {
        cursor += sizeof(value.value());
    }
    return value;
}

Optional<Bytes> ByteDecoder::peek(usize count) const
{
    auto slice = bytes.slice(cursor, count);
    if (slice.size() != count) {
        return {};
    }
    return slice;
}

Optional<StringView> ByteDecoder::peeks(usize count) const
{
    auto slice = bytes.slice(cursor, count);
    if (slice.size() != count) {
        return {};
    }
    return slice.as_view();
}

Optional<u8> ByteDecoder::peek_u8() const
{
    auto slice = bytes.slice(cursor, sizeof(u8));
    if (slice.size() != sizeof(u8)) {
        return {};
    }
    return slice[0];
}

Optional<u16> ByteDecoder::peek_u16le() const
{
    auto slice = bytes.slice(cursor, sizeof(u16));
    if (slice.size() != sizeof(u16)) {
        return {};
    }
    return device_endian_from_little(*(u16*)slice.data());
}

Optional<u32> ByteDecoder::peek_u32le() const
{
    auto slice = bytes.slice(cursor, sizeof(u32));
    if (slice.size() != sizeof(u32)) {
        return {};
    }
    return device_endian_from_little(*(u32*)slice.data());
}

Optional<u64> ByteDecoder::peek_u64le() const
{
    auto slice = bytes.slice(cursor, sizeof(u64));
    if (slice.size() != sizeof(u64)) {
        return {};
    }
    return device_endian_from_little(*(u64*)slice.data());
}

Optional<u16> ByteDecoder::peek_u16be() const
{
    auto slice = bytes.slice(cursor, sizeof(u16));
    if (slice.size() != sizeof(u16)) {
        return {};
    }
    return device_endian_from_big(*(u16*)slice.data());
}

Optional<u32> ByteDecoder::peek_u32be() const
{
    auto slice = bytes.slice(cursor, sizeof(u32));
    if (slice.size() != sizeof(u32)) {
        return {};
    }
    return device_endian_from_big(*(u32*)slice.data());
}

Optional<u64> ByteDecoder::peek_u64be() const
{
    auto slice = bytes.slice(cursor, sizeof(u64));
    if (slice.size() != sizeof(u64)) {
        return {};
    }
    return device_endian_from_big(*(u64*)slice.data());
}

Optional<StringView> ByteDecoder::parse_string(usize count)
{
    auto slice = peek(count);
    if (slice.has_value()) {
        skip(count);
    }
    return slice.map([](Bytes data) {
        return data.as_view();
    });
}

Optional<Bytes> ByteDecoder::parse_bytes(usize count)
{
    auto slice = peek(count);
    if (slice.has_value()) {
        skip(count);
    }
    return slice;
}

}
