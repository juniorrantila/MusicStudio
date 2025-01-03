#pragma once
#include "./ToggleList.h"

#include "./View.h"
#include "./Button.h"
#include "./List.h"

namespace UIView {

Vec4f ToggleList::render(UI::UI& ui, Vec4f bounds) const
{
    return list(m_items, [](Button* item) {
        return item;
    })->render(ui, bounds);
}


ToggleList* ToggleList::set_on_select(FrameCapture<void(usize id)> action)
{
    for (usize i = 0; i < m_items.size(); i++) {
        m_items[i]->action([=] {
            action(i);
        });
    }
    return self();
}

ToggleList* toggle_list(View<Button*> view)
{
    return ToggleList::make(view);
}

}
