#pragma once
#include "Base.h"
#include "Forward.h"
#include "Move.h"

namespace Ty {

template <typename Out, typename... In>
struct SmallCallableWrapperBase {
    virtual constexpr ~SmallCallableWrapperBase() = default;
    virtual constexpr Out call(In...) const = 0;

    constexpr void* storage() { return m_storage; }
    constexpr void const* storage() const { return m_storage; }

    template <typename T>
    constexpr T* storage_as()
    {
        return reinterpret_cast<T*>(m_storage);
    }

    template <typename T>
    constexpr T const* storage_as() const
    {
        return reinterpret_cast<T const*>(m_storage);
    }

    static constexpr auto storage_size = 32;
    u8 m_storage[storage_size];
};

template <typename CallableType, typename Out, typename... In>
struct SmallCallableWrapper
    : public SmallCallableWrapperBase<Out, In...> {
    using Parent = SmallCallableWrapperBase<Out, In...>;

    constexpr explicit SmallCallableWrapper(CallableType&& callable)
    {
        static_assert(sizeof(CallableType) < Parent::storage_size);
        auto& parent = *static_cast<Parent*>(this);
        new (parent.storage()) CallableType(move(callable));
    }
    SmallCallableWrapper(SmallCallableWrapper const&) = delete;
    SmallCallableWrapper& operator=(SmallCallableWrapper const&)
        = delete;
    constexpr Out call(In... in) const final
    {
        return callable()(forward<In>(in)...);
    }

    constexpr CallableType const& callable() const
    {
        return *as_parent().template storage_as<CallableType>();
    }

    constexpr Parent const& as_parent() const
    {
        return *static_cast<Parent const*>(this);
    }

    constexpr Parent& as_parent()
    {
        return *static_cast<Parent*>(this);
    }
};

template <typename Out, typename... In>
struct SmallCapture;

template <typename Out, typename... In>
struct SmallCapture<Out(In...)> {
    using Impl = SmallCallableWrapperBase<Out, In...>;

    constexpr SmallCapture() = default;
    constexpr SmallCapture(nullptr_t) { }

    template <typename F>
    constexpr SmallCapture(F&& callable) requires(
        !IsFunctionPointer<F> and IsRvalueReference<F&&>)
    {
        new (m_storage) SmallCallableWrapper<F, Out, In...>(
            forward<F>(callable));
    }

    template <typename F>
    constexpr SmallCapture(F f) requires IsFunctionPointer<F>
    {
        new (m_storage)
            SmallCallableWrapper<F, Out, In...>(forward<F>(f));
    }

    constexpr Out operator()(In... in) const
    {
        return callable_wrapper()->call(forward<In>(in)...);
    }

    template <typename F>
    constexpr SmallCapture& operator=(F&& callable) requires(
        !IsFunctionPointer<F> && IsRvalueReference<F&&>)
    {
        callable_wrapper().~Impl();
        new (m_storage) SmallCallableWrapper < F, Out,
            In... >> (forward<F>(callable));
        return *this;
    }

    template <typename F>
    constexpr SmallCapture& operator=(
        F f) requires IsFunctionPointer<F>
    {
        callable_wrapper().~Impl();
        new (m_storage) SmallCallableWrapper < F, Out,
            In... >> (forward<F>(f));
        return *this;
    }

private:
    constexpr Impl* callable_wrapper()
    {
        return reinterpret_cast<Impl*>(m_storage);
    }

    constexpr Impl const* callable_wrapper() const
    {
        return reinterpret_cast<Impl const*>(m_storage);
    }

    alignas(Impl) u8 m_storage[sizeof(Impl)];
};

}

using Ty::SmallCapture;
