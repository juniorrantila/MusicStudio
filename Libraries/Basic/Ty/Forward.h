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

template <typename>
struct SmallCapture;

template <typename T>
struct View;
struct Json;
struct Hash;

template <typename T>
struct RefPtr;

struct Coder;
struct CoderField;

struct ArenaAllocator;

}

using Ty::StringBuffer;
using Ty::Error;
using Ty::ErrorOr;
using Ty::Optional;
using Ty::Vector;
using Ty::StringView;
using Ty::Formatter;
using Ty::SmallCapture;
using Ty::View;
using Ty::Json;
using Ty::Hash;
using Ty::Coder;
using Ty::CoderField;
