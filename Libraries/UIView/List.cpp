#include "./List.h"
#include "Ty/Base.h"

#include <Rexim/LA.h>

namespace UIView {

Vec4f List::render(UI::UI& ui, Vec4f bounds) const
{
    bounds = ViewBase::render(ui, bounds);
    auto size = bounds.size();
    Vec2f start = bounds.start_point();

    if (justify_content() == SpaceBetween) {
        if (direction() == Vertical) {
            for (f64 i = 0; i < m_items.size(); i++) {
                start.y = bounds.height / f64(m_items.size()) * i;
                size.y = bounds.height / f64(m_items.size());
                if (auto h = m_items[i]->height(ui); size.y < h) {
                    size.y = h;
                }
                m_items[i]->render(ui, vec4fv(start, size));
            }
            return bounds;
        }

        for (f64 i = 0; i < m_items.size(); i++) {
            start.x = bounds.width / f64(m_items.size()) * i;
            size.x = bounds.width / f64(m_items.size());
            if (auto w = m_items[i]->width(ui); size.x < w) {
                size.x = w;
            }
            m_items[i]->render(ui, vec4fv(start, size));
        }
        return bounds;
    }

    switch (direction()) {
    case Vertical:
        for (auto* item : m_items) {
            auto height = item->height(ui);
            if (start.y + height >= bounds.height) {
                return bounds;
            }
            size.y = height;

            item->height(ui);
            item->render(ui, vec4fv(start, size));
            start.y += height + gap();
        }
        break;
    case Horizontal:
        for (auto item : m_items) {
            auto width = item->width(ui);
            if (start.x + width >= bounds.width) {
                return bounds;
            }
            size.x = width;
            item->render(ui, vec4fv(start, size));
            start.x += width + gap();
        }
        break;
    }
    return bounds;
}

f64 List::height(UI::UI& ui) const
{
    if (m_height) {
        return m_height;
    }

    switch (direction()) {
    case Horizontal: {
        f64 tallest = 0.0;
        for (auto item : m_items) {
            auto height = item->height(ui);
            if (height > tallest) {
                tallest = height;
            }
        }
        m_height = tallest;
    } break;
    case Vertical: {
        for (auto item : m_items) {
            m_height += item->height(ui) + gap();
        }
        if (!m_items.is_empty()) {
            m_height -= gap();
        }
    } break;
    }

    return m_width;
}

f64 List::width(UI::UI& ui) const
{
    if (m_width) {
        return m_width;
    }

    switch (direction()) {
    case Horizontal: {
        for (auto item : m_items) {
            m_width += item->width(ui) + gap();
        }
        if (!m_items.is_empty()) {
            m_width -= gap();
        }
    } break;
    case Vertical: {
        f64 widest = 0.0;
        for (auto item : m_items) {
            auto width = item->width(ui);
            if (width > widest) {
                widest = width;
            }
        }
        m_width = widest;
    } break;
    }

    return m_width;
}

List* list(View<ViewBase*> view)
{
    return List::make(view);
}

}
