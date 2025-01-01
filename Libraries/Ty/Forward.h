#pragma once

namespace Ty {

struct StringBuffer;
struct Error;

template <typename T, typename U = Error>
struct ErrorOr;

template <typename T>
struct Optional;

template <typename T>
struct Vector;
struct StringView;

template <typename T>
struct Formatter;

template <typename Signature>
class Function;

template <typename>
struct SmallCapture;
struct Bytes;

template <typename T>
struct View;
struct Json;
struct Hash;

template <typename T>
struct RefPtr;

template <typename T>
struct Signal;

struct Coder;
struct CoderField;

template <typename T>
struct Buffer;

struct ByteDecoder;

}

using Ty::StringBuffer;
using Ty::Error;
using Ty::ErrorOr;
using Ty::Optional;
using Ty::Vector;
using Ty::StringView;
using Ty::Formatter;
using Ty::Function;
using Ty::SmallCapture;
using Ty::Bytes;
using Ty::View;
using Ty::Json;
using Ty::Hash;
using Ty::Signal;
using Ty::Coder;
using Ty::CoderField;
using Ty::Buffer;
using Ty::ByteDecoder;
