#pragma once
#include "./Forward.h"

#include "./View.h"

#include <Rexim/LA.h>
#include <Ty/Traits.h>
#include <Ty/SmallCapture.h>

namespace UIView {

struct List : public UIView<List> {
    enum Direction {
        Vertical,
        Horizontal
    };

    enum JustifyContent {
        Begin,
        End,
        SpaceBetween,
    };

    constexpr List(View<ViewBase*> items)
        : m_items(items)
    {
    }

    Vec4f render(UI::UI& ui, Vec4f bounds) const override;

    f64 height(UI::UI&) const override;
    f64 width(UI::UI&) const override;

    virtual Direction direction() const
    {
        return m_direction;
    }

    Self* set_direction(Direction dir)
    {
        m_direction = dir;
        return self();
    }

    Self* vertical()
    {
        return set_direction(Vertical);
    }

    Self* horizontal()
    {
        return set_direction(Horizontal);
    }

    virtual f64 gap() const { return m_gap; }
    Self* set_gap(f64 gap)
    {
        m_gap = gap;
        return self();
    }

    virtual JustifyContent justify_content() const { return m_justify_content; }
    Self* set_justify_content(JustifyContent justify_content)
    {
        m_justify_content = justify_content;
        return self();
    }

    Self* space_between()
    {
        return set_justify_content(SpaceBetween);
    }

    constexpr View<ViewBase*> items() const { return m_items; }

private:
    View<ViewBase*> m_items {};
    Direction m_direction = Vertical;
    JustifyContent m_justify_content = Begin;
    f64 m_gap = 0;
};

template <typename T, typename F>
    requires IsCallableWithArguments<F, ViewBase*, T>
List* list(View<T> items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<ViewBase*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i]);
    }
    return list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, ViewBase*, T, usize>
List* list(View<T> items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<ViewBase*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i], i);
    }
    return list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, ViewBase*, T>
List* list(Vector<T> const& items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<ViewBase*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i]);
    }
    return list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, ViewBase*, T, usize>
List* list(Vector<T> const& items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<ViewBase*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i], i);
    }
    return list(slots);
}

List* list(View<ViewBase*> view);

template <usize Size>
List* list(ViewBase* const (&items)[Size])
{
    auto slots = MUST(ViewBase::alloc<ViewBase*>(Size));
    for (usize i = 0; i < slots.size(); i++) {
        slots[i] = items[i];
    }
    return list(slots);
}

}
