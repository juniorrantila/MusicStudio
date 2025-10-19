#pragma once
#include "./Types.h"

VALIDATE_IS_CPP();

#include "./Bits.h"
#include "./Verify.h"

namespace Ty3 {

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

template <typename T, typename U>
inline constexpr bool IsSame = __is_same(T, U);

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
using RemoveConst = typename remove_const<T>::Type;

template <typename T>
struct remove_const<T const> {
    using Type = T;
};

template <typename T>
struct remove_volatile{
    using Type = T;
};

template <typename T>
struct remove_volatile<T volatile> {
    using Type = T;
};

template <typename T>
using RemoveVolatile = typename remove_volatile<T>::Type;

template <typename T>
using RemoveVolatile = typename remove_volatile<T>::Type;

template <typename T>
using RemoveCVReference = RemoveConst<RemoveVolatile<RemoveReference<T>>>;

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
inline constexpr bool IsRValueReference = __is_rvalue_reference(T);

template <typename T>
inline constexpr bool IsFunctionPointer = IsPointer<T> && IsFunction<RemovePointer<T>>;

template <typename T>
concept HasInvalid = requires
{
    T::Invalid;
};

// Not a function pointer, and not an lvalue reference.
template<typename F>
inline constexpr bool IsFunctionObject = (!IsFunctionPointer<F> && IsRValueReference<F&&>);

template<typename T>
auto declval() -> T;

template<typename From, typename To>
inline constexpr bool IsConvertible = requires { declval<void (*)(To)>()(declval<From>()); };

template<class From, class To>
concept ConvertibleTo = IsConvertible<From, To>;

template<typename T, typename U>
concept SameAs = IsSame<T, U>;

template<typename T, typename Out, typename... Args>
inline constexpr bool IsCallableWithArguments = requires(T t) {
    {
        t(declval<Args>()...)
    } -> ConvertibleTo<Out>;
} || requires(T t) {
    {
        t(declval<Args>()...)
    } -> SameAs<Out>;
};

template<typename T>
constexpr T&& forward(RemoveReference<T>& param)
{
    return static_cast<T&&>(param);
}

template<typename T>
constexpr T&& forward(RemoveReference<T>&& param) noexcept
{
    static_assert(!IsLvalueReference<T>, "Can't forward an rvalue as an lvalue.");
    return static_cast<T&&>(param);
}

template<typename T>
constexpr T&& move(T& arg)
{
    return static_cast<T&&>(arg);
}

template<typename>
struct Function;

template <typename Out, typename... In>
struct Function<Out(In...)> {
    using FunctionType = Out(In...);
    using ReturnType = Out;
    using Procedure = Out(*)(void*, In...);

    class CallableWrapperBase {
    public:
        // Note: This is not const to allow storing mutable lambdas.
        virtual Out call(In...) = 0;
        virtual void init_and_swap(u8*, u64) = 0;
    };

    template<typename CallableType>
    struct CallableWrapper final : public CallableWrapperBase {
        explicit CallableWrapper(CallableType&& callable)
            : m_callable(callable)
        {
        }

        Out call(In... in) final override
        {
            return m_callable(forward<In>(in)...);
        }

        // NOLINTNEXTLINE(readability-non-const-parameter) False positive; destination is used in a placement new expression
        void init_and_swap(u8* destination, u64 size) final override
        {
            VERIFY(size >= sizeof(CallableWrapper));
            new (destination) CallableWrapper { move(m_callable) };
        }

    private:
        CallableType m_callable;
    };

    static constexpr u64 inline_alignment = alignof(CallableWrapperBase) > alignof(CallableWrapperBase*) ? alignof(CallableWrapperBase) : alignof(CallableWrapperBase*);
    static constexpr u64 inline_capacity = 3 * sizeof(void*);

    constexpr Function() = default;
    constexpr Function(decltype(nullptr))
    {
    }

    constexpr Function(Function const& other) = default;
    constexpr Function(Function&& other) = default;

    constexpr Function& operator=(Function const& other) = default;
    constexpr Function& operator=(Function&& other) = default;

    template<typename CallableType>
    constexpr Function(CallableType&& callable)
        requires(
            (IsFunctionObject<CallableType> &&
             IsCallableWithArguments<CallableType, Out, In...> &&
             !IsSame<RemoveCVReference<CallableType>, Function>
             ) && sizeof(CallableType) < inline_capacity)
    {
        init_with_callable(forward<CallableType>(callable));
    }

    template<typename CallableType>
    constexpr Function(CallableType&&)
        requires(
            (IsFunctionObject<CallableType> &&
             IsCallableWithArguments<CallableType, Out, In...> &&
             !IsSame<RemoveCVReference<CallableType>, Function>
             ) && sizeof(CallableType) >= inline_capacity)
    {
        static_assert(false, "function capture is too large, wrap them in a struct and pass it by reference instead");
    }

    template<typename FunctionType>
    constexpr Function(FunctionType f)
    requires((IsFunctionPointer<FunctionType> && IsCallableWithArguments<RemovePointer<FunctionType>, Out, In...> && !IsSame<RemoveCVReference<FunctionType>, Function>))
    {
        init_with_callable(move(f));
    }

    constexpr void* context() const { return callable_wrapper(); }

    static constexpr Out call(void* ctx, In... args)
    {
        auto wrapper = (CallableWrapperBase*)ctx;
        return wrapper->call(args...);
    }

    constexpr Procedure procedure() const { return call; }

    // Note: Despite this method being const, a mutable lambda _may_ modify its own captures.
    Out operator()(In... in) const
    {
        auto* wrapper = callable_wrapper();
        VERIFY(wrapper);
        return wrapper->call(forward<In>(in)...);
    }

    explicit operator bool() const { return !!callable_wrapper(); }

    template<typename CallableType>
    Function& operator=(CallableType&& callable)
    requires((IsFunctionObject<CallableType> && IsCallableWithArguments<CallableType, Out, In...>))
    {
        clear();
        init_with_callable(forward<CallableType>(callable));
        return *this;
    }

    template<typename FunctionType>
    Function& operator=(FunctionType f)
    requires((IsFunctionPointer<FunctionType> && IsCallableWithArguments<RemovePointer<FunctionType>, Out, In...>))
    {
        clear();
        if (f)
            init_with_callable(move(f));
        return *this;
    }

    Function& operator=(decltype(nullptr))
    {
        clear();
        return *this;
    }

    void clear()
    {
        ty_memset(callable_wrapper(), 0, sizeof(*callable_wrapper()));
    }

private:

    enum class FunctionKind : u8 {
        NullPointer,
        Inline,
    };

    CallableWrapperBase* callable_wrapper() const
    {
        return ty_bit_cast<CallableWrapperBase*>(&m_storage);
    }

    template<typename Callable>
    void init_with_callable(Callable&& callable)
    {
        using WrapperType = CallableWrapper<Callable>;
        static_assert(sizeof(WrapperType) <= inline_capacity);
        new (m_storage) WrapperType(forward<Callable>(callable));
    }

    alignas(inline_alignment) u8 m_storage[inline_capacity];
};

}

using Ty3::Function;
