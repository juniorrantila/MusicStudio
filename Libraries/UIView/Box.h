#pragma once
#include "./Forward.h"

#include "./View.h"

#include <Rexim/LA.h>
#include <Ty/Base.h>

namespace UIView {

Box* box(ViewBase* child = nullptr);

struct Box : public UIView<Box> {
    constexpr Box(ViewBase* child)
        : m_child(child)
    {
    }

    Vec4f render(UI::UI& ui, Vec4f bounds) const override;

private:
    ViewBase* m_child = nullptr;
};

}
