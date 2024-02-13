#pragma once

namespace Ty {

template <typename T>
inline constexpr bool is_trivial = __is_trivial(T);

template <typename T>
inline constexpr bool is_trivially_copyable
    = __is_trivially_copyable(T);

template <typename T>
inline constexpr bool is_trivially_destructible
    = __is_trivially_destructible(T);

template <typename T, typename U>
inline constexpr bool is_same = __is_same(T, U);

template <typename T>
inline constexpr bool is_const = __is_const(T);

template <typename T, typename... Args>
inline constexpr bool is_constructible
    = __is_constructible(T, Args...);

inline constexpr bool is_constant_evaluated()
{
    return __builtin_is_constant_evaluated();
}

template <typename T>
struct remove_reference {
    using Type = T;
};

template <typename T>
struct remove_reference<T&> {
    using Type = T;
};

template <typename T>
struct remove_reference<T&&> {
    using Type = T;
};

template <typename T>
using RemoveReference = typename remove_reference<T>::Type;

template <typename T>
struct remove_const {
    using Type = T;
};

template <typename T>
struct remove_const<T const> {
    using Type = T;
};

template <typename T>
using RemoveConst = typename remove_const<T>::Type;

template <typename T>
inline constexpr bool IsLvalueReference = false;

template <typename T>
inline constexpr bool IsLvalueReference<T&> = true;

template <typename T>
inline constexpr bool IsFunction = __is_function(T);

template <typename T>
inline constexpr bool IsPointer = __is_pointer(T);

template <typename T>
struct remove_pointer {
    using Type = T;
};

template <typename T>
struct remove_pointer<T*> {
    using Type = T;
};

template <typename T>
using RemovePointer = typename remove_pointer<T>::Type;

template <typename T>
inline constexpr bool IsRvalueReference = false;

template <typename T>
inline constexpr bool IsRvalueReference<T&&> = true;

template <typename T>
inline constexpr bool IsFunctionPointer
    = IsPointer<T>&& IsFunction<RemovePointer<T>>;

template <typename T>
concept HasInvalid = requires
{
    T::Invalid;
};

}

using namespace Ty;
