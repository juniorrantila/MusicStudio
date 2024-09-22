#include "./Style.h"

Style const& Style::the()
{
    static auto style = Style();
    return style;
}
