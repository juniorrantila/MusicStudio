#pragma once
#include "./Forward.h"

#include "./View.h"
#include "./FrameCapture.h"

namespace UIView {

struct ToggleList : UIView<ToggleList> {
    constexpr ToggleList(View<Button*> buttons)
        : m_items(buttons)
    {
    }

    Vec4f render(UI::UI&, Vec4f) const override;
    Self* set_on_select(FrameCapture<void(usize id)> action);

    Self* set_select_style(FrameCapture<void(Button*)>&& select_style)
    {
        m_select_style = move(select_style);
        return self();
    }

private:
    View<Button*> m_items {};
    FrameCapture<void(Button*)> m_select_style {};
};

ToggleList* toggle_list(View<Button*> view);

template <typename T, typename F>
    requires IsCallableWithArguments<F, Button*, T>
ToggleList* toggle_list(View<T> items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<Button*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i]);
    }
    return toggle_list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, Button*, T, usize>
ToggleList* toggle_list(View<T> items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<Button*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i], i);
    }
    return toggle_list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, Button*, T>
ToggleList* toggle_list(Vector<T> const& items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<Button*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i]);
    }
    return toggle_list(slots);
}

template <typename T, typename F>
    requires IsCallableWithArguments<F, Button*, T, usize>
ToggleList* toggle_list(Vector<T> const& items, F each_item)
{
    auto slots = MUST(ViewBase::alloc<Button*>(items.size()));
    for (usize i = 0; i < items.size(); i++) {
        slots[i] = each_item(items[i], i);
    }
    return toggle_list(slots);
}

template <usize Size>
ToggleList* toggle_list(Button* const (&items)[Size])
{
    auto slots = MUST(ViewBase::alloc<Button*>(Size));
    for (usize i = 0; i < slots.size(); i++) {
        slots[i] = items[i];
    }
    return toggle_list(slots);
}

}
