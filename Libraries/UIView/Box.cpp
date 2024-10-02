#include "./Box.h"

namespace UIView {

Vec4f Box::render(UI::UI& ui, Vec4f bounds) const
{
    bounds = ViewBase::render(ui, bounds);
    if (m_child) {
        m_child->render(ui, bounds);
    }
    return bounds;
}

Box* box(ViewBase* child)
{
    return Box::make(child);
}

}
