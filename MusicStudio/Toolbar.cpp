#include "./Toolbar.h"

#include "./Style.h"

#include <UIView/Box.h>

using UIView::box;

UIView::ViewBase* toolbar()
{
    return box()
        ->set_height(48.0)
        ->set_border_bottom_width(Style::the().border_size())
        ->set_border_bottom_color(Style::the().toolbar_border_color())
        ->set_background_color(Style::the().toolbar_color());
}
