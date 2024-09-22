#pragma once
#include <Ty/SmallVector.h>
#include <Ty/Optional.h>
#include <Ty/TypeId.h>
#include <Ty/SmallCapture.h>
#include <Ty/Vector.h>

struct EventIterator;
struct Event;

static inline u16 next_id()
{
    static u16 id = 0;
    return id++;
}

struct Event;
struct EventMatch {
    EventMatch(TypeId id)
        : m_id(id)
    {
    }

    virtual void on_match(Event) const = 0;
    TypeId id() const { return m_id; }

private:
    TypeId m_id;
};

template<typename T>
concept IsEventMatch = IsConvertible<T const&, EventMatch const&>;

struct Event {
    void const* data;
    TypeId kind;

    constexpr void match(IsEventMatch auto const&... args) const
    {
        constexpr usize size = sizeof...(args);
        EventMatch const* const matchers[size] = {
            &args...
        };

        for (usize i = 0; i < size; i++) {
            if (kind == matchers[i]->id()) {
                matchers[i]->on_match(*this);
                return;
            }
        }
    }
};

template <typename T>
struct On : EventMatch {
    On(SmallCapture<void(T const&)> match)
        : EventMatch(type_id<T>())
        , m_match(move(match))
    {
    }

    void on_match(Event event) const override
    {
        m_match(*(T const*)event.data);
    }

private:
    SmallCapture<void(T const&)> m_match;
};

struct EventLoop {
    friend EventIterator;

    EventIterator events();

    template <typename T>
    ErrorOr<void> dispatch_event(T event)
    {
        VERIFY(((uptr)m_event_data.data()) % alignof(T) == 0);

        while (m_event_data.size() % alignof(T))
            TRY(m_event_data.append(0));

        u32 index = m_event_data.size();
        for (u32 i = 0; i < sizeof(T); i++)
            TRY(m_event_data.append(0));

        T* event_ptr = (T*)&m_event_data[index];
        new (event_ptr) T(move(event));
        TRY(m_destructors.append([event_ptr] {
            event_ptr->~T();
        }));

        TRY(m_events.append({
            .data = event_ptr,
            .kind = type_id<T>(),
        }));

        return {};
    }

private:
    void clear()
    {
        for (auto const& destructor : m_destructors)
            destructor();
        m_events.clear();
        m_event_data.clear();
        m_destructors.clear();
    }

    SmallVector<Event, 128> m_events {};
    Vector<u8> m_event_data {};
    Vector<SmallCapture<void()>> m_destructors {};
};

struct EventIterator {
    constexpr EventIterator(EventLoop& event_loop)
        : m_event_loop(event_loop)
    {
    }

    constexpr EventIterator begin() const;
    constexpr EventIterator end() const;
    constexpr Event const& operator*() const;
    constexpr EventIterator operator++();
    constexpr bool operator!=(EventIterator const& other);

private:
    constexpr EventIterator(EventLoop& event_loop, u32 event)
        : m_event_loop(event_loop)
        , m_event(event)
    {
    }

    EventLoop& m_event_loop;
    u32 m_event { 0 };
};


inline EventIterator EventLoop::events()
{
    return EventIterator(*this);
}

constexpr EventIterator EventIterator::begin() const
{
    return {
        m_event_loop,
        0,
    };
}

constexpr EventIterator EventIterator::end() const
{
    return {
        m_event_loop,
        m_event_loop.m_events.size(),
    };
}

constexpr Event const& EventIterator::operator*() const
{
    return m_event_loop.m_events[m_event];
}

constexpr EventIterator EventIterator::operator++()
{
    return {
        m_event_loop,
        ++m_event,
    };
}

constexpr bool EventIterator::operator!=(EventIterator const& other)
{
    if (m_event != other.m_event_loop.m_events.size())
        return true;
    m_event_loop.clear();
    return false;
}
