#pragma once
#include "./Forward.h"
#include "./Vector.h"
#include "./SmallCapture.h"

namespace Ty {

template <typename T>
struct Signal {
    struct Subscribe;
    using Subscriber = Id<Subscribe>;

    template <typename... Args>
    constexpr Signal(Args... args)
        : m_value(T(args...))
    {
    }

    Signal(Signal&& other) = default;

    constexpr ErrorOr<Subscriber> add_observer(SmallCapture<void(T const&)>&& on_update)
    {
        for (usize i = 0; i < m_subscribers.size(); i++) {
            auto& subscriber = m_subscribers[i];
            if (!subscriber) {
                subscriber = move(on_update);
                return Subscriber(i);
            }
        }
        auto id = TRY(m_subscribers.append(move(on_update)));
        return Subscriber(id.raw());
    }

    void remove_observer(Subscriber id)
    {
        m_subscribers[id.raw()] = nullptr;
    }

    template <typename F>
    constexpr Signal& update(F callback)
        requires IsCallableWithArguments<F, void, T&>
    {
        callback(m_value);
        notify();
        return *this;
    }

    Signal& update(T value)
    {
        m_value = move(value);
        notify();
        return *this;
    }

    explicit operator T const&() const { return value(); }
    T const& value() const { return m_value; }
    T const* operator->() const { return &m_value; }

    template <typename I>
    decltype(auto) operator[](I index)
    {
        return m_value[index];
    }


private:
    void notify()
    {
        for (auto const& callback : m_subscribers) {
            if (callback) {
                callback(m_value);
            }
        }
    }

    Vector<SmallCapture<void(T const&)>> m_subscribers {};
    T m_value;
};

}
