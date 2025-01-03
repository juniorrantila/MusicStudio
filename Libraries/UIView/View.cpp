#include "./View.h"

#include <Ty/ArenaAllocator.h>
#include <Ty/System.h>
#include <UI/UI.h>

namespace UIView {

Vec4f ViewBase::render(UI::UI& ui, Vec4f bounds) const
{
    // Vec2f size = bounds.size();
    // auto w = width(ui);
    // if (w < size.x) {
    //     bounds.width = w;
    // }

    // auto h = height(ui);
    // if (h < size.y) {
    //     bounds.height = h;
    // }

    // Border left
    auto left = border_left_width();
    ui.fill_rect(
        vec4fv(
            bounds.start_point(),
            vec2f(left, bounds.height)
        ),
        border_left_color()
    );
    bounds.x += left;
    bounds.width -= left;

    // Border bottom
    auto bottom = border_bottom_width();
    ui.fill_rect(
        vec4fv(
            bounds.start_point(),
            vec2f(bounds.width, bottom)
        ),
        border_bottom_color()
    );
    bounds.y += bottom;
    bounds.height -= bottom;

    ui.fill_rect(bounds, background_color());

    return bounds;
}

ErrorOr<void> ViewBase::init(usize max_memory)
{
    s_pool = TRY(ArenaAllocator::create(max_memory));
    return {};
}

ErrorOr<void*> ViewBase::alloc(usize size, usize align, c_string function, c_string file, usize line)
{
    if (!s_pool.is_initialized()) {
        TRY(init());
    }

    return TRY(s_pool.raw_alloc(size, align, function, file, line));
}

void ViewBase::drain()
{
    s_pool.drain();
}

}
