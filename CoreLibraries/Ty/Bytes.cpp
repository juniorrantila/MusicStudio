#include "./Bytes.h"
#include "./StringView.h"

namespace Ty {

StringView Bytes::as_view() const
{
    return StringView::from_parts((char const*)m_data, m_size);
}

}
