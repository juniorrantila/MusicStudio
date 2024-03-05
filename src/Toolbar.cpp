#include "./Toolbar.h"

#include "./Style.h"

#include <UI/UI.h>

void Toolbar::render(UI::UI& ui, EventLoop&, Vec4f box)
{
    ui.outline_rect({
        .box = box,
        .outline_size = Style::the().border_size(),
        .fill_color   = Style::the().toolbar_color(),
        .bottom_color = Style::the().toolbar_border_color(),
    });
}
